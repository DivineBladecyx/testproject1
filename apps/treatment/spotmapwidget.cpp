#include "spotmapwidget.h"

#include <QJsonArray>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QtMath>
#include <QToolTip>
#include <cmath>

#include <algorithm>

namespace {

int jsonInt(const QJsonObject &object, const QStringList &keys, int fallback = -1)
{
    for (const QString &key : keys) {
        const QJsonValue value = object.value(key);
        if (value.isDouble()) {
            return value.toInt();
        }
    }
    return fallback;
}

double jsonDouble(const QJsonObject &object, const QStringList &keys, double fallback = -1.0)
{
    for (const QString &key : keys) {
        const QJsonValue value = object.value(key);
        if (value.isDouble()) {
            return value.toDouble();
        }
    }
    return fallback;
}

QVector<QPointF> readSpotPositions(const QJsonArray &values)
{
    QVector<QPointF> positions;
    if (values.isEmpty()) {
        return positions;
    }

    if (values.first().isArray()) {
        positions.reserve(values.size());
        for (const QJsonValue &value : values) {
            const QJsonArray pair = value.toArray();
            if (pair.size() >= 2 && pair.at(0).isDouble() && pair.at(1).isDouble()) {
                positions.append(QPointF(pair.at(0).toDouble(), pair.at(1).toDouble()));
            } else positions.append(QPointF(qQNaN(), qQNaN()));
        }
        return positions;
    }

    positions.reserve(values.size() / 2);
    for (qsizetype index = 0; index + 1 < values.size(); index += 2) {
        if (values.at(index).isDouble() && values.at(index + 1).isDouble()) {
            positions.append(QPointF(values.at(index).toDouble(), values.at(index + 1).toDouble()));
        } else positions.append(QPointF(qQNaN(), qQNaN()));
    }
    return positions;
}

QJsonArray normalizedLayerGroups(const QJsonObject &payload)
{
    const QJsonArray rawLayers = payload.value(QStringLiteral("layers")).toArray();
    if (rawLayers.isEmpty()) {
        return {};
    }

    if (rawLayers.first().isArray()) {
        return rawLayers;
    }

    if (rawLayers.first().isObject()) {
        QJsonArray singleBeamLayers;
        singleBeamLayers.append(rawLayers);
        return singleBeamLayers;
    }

    return {};
}

} // namespace

SpotMapWidget::SpotMapWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("spotMapWidget"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumHeight(250);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setMouseTracking(true);
    setToolTip(tr("束斑计划视图；滚轮缩放，拖动平移，双击复位。悬浮数值为计划权重，不是实测剂量。"));
}

QSize SpotMapWidget::minimumSizeHint() const
{
    return QSize(420, 250);
}

QJsonObject SpotMapWidget::planSummary(const QJsonObject &payload) const
{
    QJsonObject summary = payload.value(QStringLiteral("plan_summary")).toObject();
    if (!summary.isEmpty()) {
        return summary;
    }

    const QJsonObject planPackage = payload.value(QStringLiteral("plan_package")).toObject();
    return planPackage.value(QStringLiteral("summary_json")).toObject();
}

void SpotMapWidget::setPlanPayload(const QJsonObject &payload)
{
    m_beams.clear();
    m_beamIndex = 0;
    m_layerIndex = 0;
    m_currentLayerNumber = -1;
    m_isDelivering = false;
    m_planZoom = 1.0;
    m_planPan = QPointF();
    m_hoveredSpot = -1;
    QToolTip::hideText();

    const QJsonArray summaryBeams =
        planSummary(payload).value(QStringLiteral("beams")).toArray();
    const QJsonArray layerGroups = normalizedLayerGroups(payload);
    const qsizetype beamCount = qMax(summaryBeams.size(), layerGroups.size());
    m_beams.reserve(beamCount);
    for (qsizetype beamPosition = 0; beamPosition < beamCount; ++beamPosition) {
        const QJsonObject beamObject = beamPosition < summaryBeams.size()
        ? summaryBeams.at(beamPosition).toObject()
        : QJsonObject();
        BeamData beam;
        beam.number = beamObject.value(QStringLiteral("beam_number"))
                          .toInt(static_cast<int>(beamPosition + 1));
        beam.name = beamObject.value(QStringLiteral("beam_name")).toString();
        beam.declaredLayerCount = beamObject.value(QStringLiteral("energy_layers")).toInt();
        beam.declaredSpotCount = beamObject.value(QStringLiteral("total_scan_spots")).toInt();

        QJsonArray layers;
        if (beamPosition < layerGroups.size()) {
            layers = layerGroups.at(beamPosition).toArray();
        }
        if (layers.isEmpty()) {
            layers = beamObject.value(QStringLiteral("layers")).toArray();
        }
        if (!layers.isEmpty()) {
            beam.declaredLayerCount = layers.size();
        }

        beam.layers.reserve(layers.size());
        int parsedSpotCount = 0;
        for (qsizetype layerPosition = 0; layerPosition < layers.size(); ++layerPosition) {
            const QJsonObject layerObject = layers.at(layerPosition).toObject();
            LayerData layer;
            layer.index = layerObject.value(QStringLiteral("layer_index"))
                              .toInt(static_cast<int>(layerPosition + 1));
            layer.energyMev = layerObject.value(QStringLiteral("nominal_energy_mev")).toDouble(-1.0);

            const QVector<QPointF> positions = readSpotPositions(
                layerObject.value(QStringLiteral("spot_positions_mm")).toArray());
            const QJsonArray weights = layerObject.value(QStringLiteral("spot_meterset_weights")).toArray();
            const qsizetype validSpotCount = qMin(positions.size(), weights.size());
            layer.spots.reserve(validSpotCount);
            for (qsizetype spotIndex = 0; spotIndex < validSpotCount; ++spotIndex) {
                if (!weights.at(spotIndex).isDouble()
                    || !std::isfinite(weights.at(spotIndex).toDouble())
                    || weights.at(spotIndex).toDouble() < 0.0
                    || !std::isfinite(positions.at(spotIndex).x())
                    || !std::isfinite(positions.at(spotIndex).y())) {
                    continue;
                }
                SpotPoint spot;
                spot.position = positions.at(spotIndex);
                spot.weight = weights.at(spotIndex).toDouble();
                spot.sourceIndex = static_cast<int>(spotIndex);
                layer.spots.append(spot);
            }
            parsedSpotCount += layer.spots.size();
            beam.layers.append(layer);
        }
        if (parsedSpotCount > 0) {
            beam.declaredSpotCount = parsedSpotCount;
        }
        m_beams.append(beam);
    }

    invalidatePlanCache();
    invalidatePointCloudCache();
    update();
}

void SpotMapWidget::clearPlan()
{
    m_beams.clear();
    m_beamIndex = 0;
    m_layerIndex = 0;
    m_currentLayerNumber = -1;
    m_isDelivering = false;
    m_planZoom = 1.0;
    m_planPan = QPointF();
    m_hoveredSpot = -1;
    QToolTip::hideText();
    invalidatePlanCache();
    invalidatePointCloudCache();
    update();
}

int SpotMapWidget::findBeamIndex(int beamNumber) const
{
    if (beamNumber < 0) {
        return m_beamIndex;
    }
    for (qsizetype index = 0; index < m_beams.size(); ++index) {
        if (m_beams.at(index).number == beamNumber) {
            return static_cast<int>(index);
        }
    }
    return m_beamIndex;
}

int SpotMapWidget::findLayerIndex(const BeamData &beam, int layerNumber) const
{
    if (layerNumber < 0 || beam.layers.isEmpty()) {
        return m_layerIndex;
    }

    const bool planUsesZeroBasedLayerIndex = beam.layers.first().index == 0;
    const int targetLayerIndex = planUsesZeroBasedLayerIndex && layerNumber > 0
                                     ? layerNumber - 1
                                     : layerNumber;
    for (qsizetype index = 0; index < beam.layers.size(); ++index) {
        if (beam.layers.at(index).index == targetLayerIndex) {
            return static_cast<int>(index);
        }
    }
    return qBound(0, layerNumber - 1, static_cast<int>(beam.layers.size()) - 1);
}

void SpotMapWidget::setTelemetry(const QJsonObject &payload)
{
    if (m_beams.isEmpty()) {
        return;
    }

    const int beamNumber = jsonInt(payload,
                                   {QStringLiteral("current_beam"),
                                    QStringLiteral("current_beam_number"),
                                    QStringLiteral("beam_number")});
    const int nextBeamIndex = findBeamIndex(beamNumber);
    bool planLayerChanged = nextBeamIndex != m_beamIndex;
    m_beamIndex = nextBeamIndex;

    const BeamData &beam = m_beams.at(m_beamIndex);
    const int layerNumber = jsonInt(payload, {QStringLiteral("current_layer")});
    const int nextLayerIndex = findLayerIndex(beam, layerNumber);
    planLayerChanged = planLayerChanged || nextLayerIndex != m_layerIndex;
    m_layerIndex = nextLayerIndex;
    const bool layerNumberChanged = layerNumber >= 0
                                    && layerNumber != m_currentLayerNumber;

    const bool nextDelivering = !payload.value(QStringLiteral("beam_on_is_snapshot")).toBool()
                                && (payload.contains(QStringLiteral("beam_on"))
                                        ? payload.value(QStringLiteral("beam_on")).toBool()
                                        : payload.contains(QStringLiteral("is_delivering"))
                                              ? payload.value(QStringLiteral("is_delivering")).toBool()
                                              : m_isDelivering);

    const bool changed = planLayerChanged
                         || layerNumberChanged
                         || nextDelivering != m_isDelivering;
    if (layerNumber >= 0) {
        m_currentLayerNumber = layerNumber;
    }
    m_isDelivering = nextDelivering;

    if (planLayerChanged) {
        m_hoveredSpot = -1;
        QToolTip::hideText();
        invalidatePlanCache();
    }
    if (planLayerChanged || layerNumberChanged) {
        invalidatePointCloudCache();
    }
    if (changed) {
        update();
    }
}

QRect SpotMapWidget::plotRect() const
{
    return rect().adjusted(12, 76, -12, -34);
}

QRect SpotMapWidget::mode2DRect() const
{
    return QRect(width() - 138, 9, 58, 26);
}

QRect SpotMapWidget::mode3DRect() const
{
    return QRect(width() - 76, 9, 64, 26);
}

QRect SpotMapWidget::topViewRect() const
{
    return QRect(width() - 278, 9, 64, 26);
}

QRect SpotMapWidget::sideViewRect() const
{
    return QRect(width() - 210, 9, 64, 26);
}

void SpotMapWidget::invalidatePlanCache()
{
    m_planCache = QPixmap();
    m_cachedPlotSize = QSize();
    m_spotBounds = QRectF();
}

void SpotMapWidget::invalidatePointCloudCache()
{
    m_pointCloudCache = QPixmap();
    m_cachedPointCloudSize = QSize();
}

QPointF SpotMapWidget::mapSpotToPlot(const QPointF &point) const
{
    const QRect target(QPoint(0, 0), m_cachedPlotSize);
    const double width = qMax(1.0, m_spotBounds.width());
    const double height = qMax(1.0, m_spotBounds.height());
    const double scale = qMin((target.width() - 32.0) / width,
                              (target.height() - 32.0) / height) * m_planZoom;
    const QPointF center = m_spotBounds.center();
    return QPointF(target.center().x() + (point.x() - center.x()) * scale + m_planPan.x(),
                   target.center().y() - (point.y() - center.y()) * scale + m_planPan.y());
}

int SpotMapWidget::spotIndexAt(const QPoint &position) const
{
    if (m_viewMode != ViewMode::Plan2D || !plotRect().contains(position)
        || m_beams.isEmpty() || m_planCache.isNull()) return -1;
    const BeamData &beam = m_beams.at(m_beamIndex);
    if (beam.layers.isEmpty()) return -1;
    const auto &spots = beam.layers.at(m_layerIndex).spots;
    const QPointF local = position - plotRect().topLeft();
    double nearest = 100.0;
    int hit = -1;
    for (int index = 0; index < spots.size(); ++index) {
        const QPointF delta = mapSpotToPlot(spots.at(index).position) - local;
        const double distance = delta.x() * delta.x() + delta.y() * delta.y();
        if (distance <= nearest) { nearest = distance; hit = index; }
    }
    return hit;
}

QString SpotMapWidget::spotInfoAt(const QPoint &position) const
{
    const int index = spotIndexAt(position);
    if (index < 0) return QString();
    const BeamData &beam = m_beams.at(m_beamIndex);
    const LayerData &layer = beam.layers.at(m_layerIndex);
    const SpotPoint &spot = layer.spots.at(index);
    return tr("射野 %1 · 能层序号 %2 · Spot %3\nX %4 mm / Y %5 mm\n计划权重 %6\nspot_meterset_weights：不是实测剂量，不换算为 Gy")
        .arg(beam.number).arg(m_layerIndex + 1).arg(spot.sourceIndex + 1)
        .arg(spot.position.x(), 0, 'f', 2).arg(spot.position.y(), 0, 'f', 2)
        .arg(spot.weight, 0, 'g', 8);
}

void SpotMapWidget::rebuildPlanCache()
{
    const QRect area = plotRect();
    if (area.width() <= 0 || area.height() <= 0 || m_beams.isEmpty()) {
        return;
    }

    const BeamData &beam = m_beams.at(qBound(0, m_beamIndex, m_beams.size() - 1));
    if (beam.layers.isEmpty()) {
        return;
    }
    const LayerData &layer = beam.layers.at(qBound(0, m_layerIndex, beam.layers.size() - 1));
    if (layer.spots.isEmpty()) {
        return;
    }

    double minX = layer.spots.first().position.x();
    double maxX = minX;
    double minY = layer.spots.first().position.y();
    double maxY = minY;
    double maxWeight = 0.0;
    for (const SpotPoint &spot : layer.spots) {
        minX = qMin(minX, spot.position.x());
        maxX = qMax(maxX, spot.position.x());
        minY = qMin(minY, spot.position.y());
        maxY = qMax(maxY, spot.position.y());
        maxWeight = qMax(maxWeight, spot.weight);
    }
    if (qFuzzyCompare(minX, maxX)) {
        minX -= 1.0;
        maxX += 1.0;
    }
    if (qFuzzyCompare(minY, maxY)) {
        minY -= 1.0;
        maxY += 1.0;
    }
    m_spotBounds = QRectF(QPointF(minX, minY), QPointF(maxX, maxY)).normalized();
    m_cachedPlotSize = area.size();

    const qreal ratio = devicePixelRatioF();
    m_planCache = QPixmap(m_cachedPlotSize * ratio);
    m_planCache.setDevicePixelRatio(ratio);
    m_planCache.fill(QColor(QStringLiteral("#082E53")));

    QPainter painter(&m_planCache);
    painter.setRenderHint(QPainter::Antialiasing, layer.spots.size() < 2500);
    painter.setPen(Qt::NoPen);
    const qreal baseRadius = layer.spots.size() > 2500 ? 1.1 : 2.2;
    for (const SpotPoint &spot : layer.spots) {
        const double normalizedWeight = maxWeight > 0.0 ? spot.weight / maxWeight : 0.65;
        QColor color(QStringLiteral("#37A8FF"));
        color.setAlpha(120 + qRound(135.0 * qBound(0.0, normalizedWeight, 1.0)));
        painter.setBrush(color);
        const qreal radius = baseRadius + normalizedWeight * 1.4;
        painter.drawEllipse(mapSpotToPlot(spot.position), radius, radius);
    }
}

QPointF SpotMapWidget::projectPoint(double x,
                                    double y,
                                    double z,
                                    const QRect &target,
                                    const QRectF &xyBounds,
                                    int layerCount) const
{
    const double xySpan = qMax(1.0, qMax(xyBounds.width(), xyBounds.height()));
    const double nx = (x - xyBounds.center().x()) / xySpan * 2.0;
    const double ny = (y - xyBounds.center().y()) / xySpan * 2.0;
    const double nz = layerCount > 1
                          ? ((z / static_cast<double>(layerCount - 1)) - 0.5) * 1.35
                          : 0.0;

    const double yaw = qDegreesToRadians(m_yawDegrees);
    const double pitch = qDegreesToRadians(m_pitchDegrees);
    const double rotatedX = qCos(yaw) * nx - qSin(yaw) * ny;
    const double rotatedY = qSin(yaw) * nx + qCos(yaw) * ny;
    const double screenY = qCos(pitch) * rotatedY - qSin(pitch) * nz;
    const double scale = qMin(target.width(), target.height()) * 0.34 * m_zoom;

    return QPointF(target.center().x() + rotatedX * scale,
                   target.center().y() - screenY * scale);
}

void SpotMapWidget::rebuildPointCloudCache()
{
    const QRect area = plotRect();
    if (area.width() <= 0 || area.height() <= 0 || m_beams.isEmpty()) {
        return;
    }

    const BeamData &beam = m_beams.at(qBound(0, m_beamIndex, m_beams.size() - 1));
    if (beam.layers.isEmpty()) {
        return;
    }

    bool hasSpot = false;
    double minX = 0.0;
    double maxX = 0.0;
    double minY = 0.0;
    double maxY = 0.0;
    double maxWeight = 0.0;
    for (const LayerData &layer : beam.layers) {
        for (const SpotPoint &spot : layer.spots) {
            if (!hasSpot) {
                minX = maxX = spot.position.x();
                minY = maxY = spot.position.y();
                hasSpot = true;
            } else {
                minX = qMin(minX, spot.position.x());
                maxX = qMax(maxX, spot.position.x());
                minY = qMin(minY, spot.position.y());
                maxY = qMax(maxY, spot.position.y());
            }
            maxWeight = qMax(maxWeight, spot.weight);
        }
    }
    if (!hasSpot) {
        return;
    }

    if (qFuzzyCompare(minX, maxX)) {
        minX -= 1.0;
        maxX += 1.0;
    }
    if (qFuzzyCompare(minY, maxY)) {
        minY -= 1.0;
        maxY += 1.0;
    }
    const QRectF xyBounds(QPointF(minX, minY), QPointF(maxX, maxY));

    struct RenderPoint {
        QPointF position;
        double depth = 0.0;
        double weight = 0.0;
        int layerPosition = 0;
    };
    QVector<RenderPoint> renderPoints;
    renderPoints.reserve(beam.declaredSpotCount > 0 ? beam.declaredSpotCount : 10000);

    const double yaw = qDegreesToRadians(m_yawDegrees);
    const double pitch = qDegreesToRadians(m_pitchDegrees);
    const double xySpan = qMax(1.0, qMax(xyBounds.width(), xyBounds.height()));
    for (qsizetype layerPosition = 0;
         layerPosition < beam.layers.size();
         ++layerPosition) {
        const LayerData &layer = beam.layers.at(layerPosition);
        for (const SpotPoint &spot : layer.spots) {
            const double nx =
                (spot.position.x() - xyBounds.center().x()) / xySpan * 2.0;
            const double ny =
                (spot.position.y() - xyBounds.center().y()) / xySpan * 2.0;
            const double nz = beam.layers.size() > 1
                                  ? ((static_cast<double>(layerPosition)
                                      / static_cast<double>(beam.layers.size() - 1)) - 0.5) * 1.35
                                  : 0.0;
            const double rotatedY = qSin(yaw) * nx + qCos(yaw) * ny;
            RenderPoint point;
            point.position = projectPoint(spot.position.x(),
                                          spot.position.y(),
                                          layerPosition,
                                          QRect(QPoint(0, 0), area.size()),
                                          xyBounds,
                                          beam.layers.size());
            point.depth = qSin(pitch) * rotatedY + qCos(pitch) * nz;
            point.weight = spot.weight;
            point.layerPosition = static_cast<int>(layerPosition);
            renderPoints.append(point);
        }
    }
    std::sort(renderPoints.begin(), renderPoints.end(),
              [](const RenderPoint &left, const RenderPoint &right) {
                  return left.depth < right.depth;
              });

    m_cachedPointCloudSize = area.size();
    const qreal ratio = devicePixelRatioF();
    m_pointCloudCache = QPixmap(m_cachedPointCloudSize * ratio);
    m_pointCloudCache.setDevicePixelRatio(ratio);
    m_pointCloudCache.fill(QColor(QStringLiteral("#071F38")));

    QPainter painter(&m_pointCloudCache);
    painter.setRenderHint(QPainter::Antialiasing, renderPoints.size() < 16000);

    const QRect localArea(QPoint(0, 0), area.size());
    painter.setPen(QPen(QColor(82, 123, 157, 110), 1));
    const QPointF origin = projectPoint(xyBounds.center().x(),
                                        xyBounds.center().y(),
                                        0,
                                        localArea,
                                        xyBounds,
                                        beam.layers.size());
    const QPointF xAxis = projectPoint(maxX,
                                       xyBounds.center().y(),
                                       0,
                                       localArea,
                                       xyBounds,
                                       beam.layers.size());
    const QPointF yAxis = projectPoint(xyBounds.center().x(),
                                       maxY,
                                       0,
                                       localArea,
                                       xyBounds,
                                       beam.layers.size());
    const QPointF zAxis = projectPoint(xyBounds.center().x(),
                                       xyBounds.center().y(),
                                       beam.layers.size() - 1,
                                       localArea,
                                       xyBounds,
                                       beam.layers.size());
    painter.drawLine(origin, xAxis);
    painter.drawLine(origin, yAxis);
    painter.drawLine(origin, zAxis);

    painter.setPen(Qt::NoPen);
    for (const RenderPoint &point : renderPoints) {
        const bool completed = m_currentLayerNumber > 0
                               && point.layerPosition < m_layerIndex;
        const bool current = point.layerPosition == m_layerIndex;
        QColor color;
        if (current) {
            color = QColor(QStringLiteral("#FFD34D"));
        } else if (completed) {
            color = QColor(QStringLiteral("#36C98F"));
            color.setAlpha(155);
        } else {
            color = QColor(QStringLiteral("#3B8CFF"));
            color.setAlpha(135);
        }
        painter.setBrush(color);
        const double normalizedWeight =
            maxWeight > 0.0 ? qBound(0.0, point.weight / maxWeight, 1.0) : 0.5;
        const qreal radius = 1.0 + normalizedWeight * 1.8;
        painter.drawEllipse(point.position, radius, radius);
    }
}

QString SpotMapWidget::statusText() const
{
    if (m_beams.isEmpty()) {
        return tr("等待治疗计划");
    }
    const BeamData &beam = m_beams.at(qBound(0, m_beamIndex, m_beams.size() - 1));
    if (beam.layers.isEmpty()) {
        return tr("计划未提供逐点束斑坐标");
    }
    return m_isDelivering ? tr("照射中") : tr("计划预览");
}

void SpotMapWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), palette().color(QPalette::Window));

    painter.setPen(palette().color(QPalette::WindowText));
    QFont titleFont = font();
    titleFont.setPixelSize(20);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.drawText(QRect(12, 10, width() - 24, 24), Qt::AlignLeft | Qt::AlignVCenter,
                     tr("计划束斑图"));

    QString summaryText = statusText();
    if (!m_beams.isEmpty()) {
        const BeamData &beam = m_beams.at(qBound(0, m_beamIndex, m_beams.size() - 1));
        const int layerCount = !beam.layers.isEmpty() ? beam.layers.size() : beam.declaredLayerCount;
        summaryText = tr("Beam %1  |  %2 能层  |  %3 Spot")
                          .arg(beam.number > 0 ? beam.number : m_beamIndex + 1)
                          .arg(layerCount > 0 ? QString::number(layerCount) : QStringLiteral("-"))
                          .arg(beam.declaredSpotCount > 0
                                   ? QString::number(beam.declaredSpotCount)
                                   : QStringLiteral("-"));
    }
    painter.setPen(palette().color(QPalette::WindowText));
    QFont infoFont = font();
    infoFont.setPixelSize(16);
    painter.setFont(infoFont);
    painter.drawText(QRect(12, 40, width() - 24, 28),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     summaryText);

    const auto drawViewButton = [&painter, this](const QRect &buttonRect,
                                                 const QString &text,
                                                 bool selected) {
        painter.setPen(QPen(QColor(selected ? QStringLiteral("#2F80ED")
                                            : QStringLiteral("#C7D8EA")), 1));
        painter.setBrush(palette().color(selected ? QPalette::Highlight : QPalette::Window));
        painter.drawRoundedRect(buttonRect, 4, 4);
        painter.setPen(palette().color(selected ? QPalette::HighlightedText : QPalette::WindowText));
        painter.drawText(buttonRect, Qt::AlignCenter, text);
    };
    if (m_viewMode == ViewMode::PointCloud3D) {
        drawViewButton(topViewRect(), tr("俯视"), qFuzzyIsNull(m_pitchDegrees));
        drawViewButton(sideViewRect(), tr("侧视"),
                       qAbs(qAbs(m_pitchDegrees) - 90.0) < 0.5);
    }
    drawViewButton(mode2DRect(), tr("二维"), m_viewMode == ViewMode::Plan2D);
    drawViewButton(mode3DRect(), tr("三维"), m_viewMode == ViewMode::PointCloud3D);

    const QRect area = plotRect();
    painter.setPen(QPen(QColor(QStringLiteral("#CFE0F1")), 1));
    painter.setBrush(palette().color(QPalette::Window));
    painter.drawRoundedRect(area, 5, 5);

    if (m_viewMode == ViewMode::PointCloud3D) {
        if (m_beams.isEmpty()) {
            painter.setPen(palette().color(QPalette::WindowText));
            painter.drawText(area, Qt::AlignCenter, tr("等待治疗计划"));
        } else {
            const BeamData &beam =
                m_beams.at(qBound(0, m_beamIndex, m_beams.size() - 1));
            if (beam.layers.isEmpty()) {
                painter.setPen(QColor(QStringLiteral("#8A6A1F")));
                painter.drawText(area, Qt::AlignCenter,
                                 tr("计划未提供三维束斑数据"));
            } else {
                if (m_pointCloudCache.isNull()
                    || m_cachedPointCloudSize != area.size()) {
                    rebuildPointCloudCache();
                }
                if (!m_pointCloudCache.isNull()) {
                    painter.drawPixmap(area.topLeft(), m_pointCloudCache);
                }
            }
        }
    } else if (m_beams.isEmpty()) {
        painter.setPen(palette().color(QPalette::WindowText));
        painter.drawText(area, Qt::AlignCenter, tr("等待治疗计划"));
    } else {
        const BeamData &beam = m_beams.at(qBound(0, m_beamIndex, m_beams.size() - 1));
        if (beam.layers.isEmpty()) {
            painter.setPen(QColor(QStringLiteral("#8A6A1F")));
            painter.drawText(area, Qt::AlignCenter,
                             tr("计划未提供 layers[].spot_positions_mm\n暂时无法绘制束斑位置"));
        } else {
            const LayerData &layer = beam.layers.at(qBound(0, m_layerIndex, beam.layers.size() - 1));
            if (layer.spots.isEmpty()) {
                painter.setPen(QColor(QStringLiteral("#8A6A1F")));
                painter.drawText(area, Qt::AlignCenter, tr("当前能层没有有效束斑坐标"));
            } else {
                if (m_planCache.isNull() || m_cachedPlotSize != area.size()) {
                    rebuildPlanCache();
                }
                painter.drawPixmap(area.topLeft(), m_planCache);
                if (m_hoveredSpot >= 0 && m_hoveredSpot < layer.spots.size()) {
                    painter.save();
                    painter.setClipRect(area);
                    painter.setPen(QPen(QColor(QStringLiteral("#FFFFFF")), 2));
                    painter.setBrush(Qt::NoBrush);
                    painter.drawEllipse(mapSpotToPlot(layer.spots.at(m_hoveredSpot).position)
                                            + area.topLeft(), 7, 7);
                    painter.restore();
                }

                if (m_isDelivering) {
                    painter.setPen(QPen(QColor(QStringLiteral("#FFC847")), 2));
                    painter.setBrush(Qt::NoBrush);
                    painter.drawRoundedRect(area.adjusted(1, 1, -1, -1), 5, 5);
                }
            }
        }
    }

    painter.setFont(infoFont);
    painter.setPen(palette().color(QPalette::WindowText));
    QString footer = statusText();
    if (!m_beams.isEmpty() && !m_beams.at(m_beamIndex).layers.isEmpty()) {
        const LayerData &layer = m_beams.at(m_beamIndex).layers.at(m_layerIndex);
        const int displayedLayer = m_currentLayerNumber > 0
                                       ? m_currentLayerNumber
                                       : m_layerIndex + 1;
        footer = (m_currentLayerNumber >= 0
                      ? tr("回读能层：%1 / %2    本层 Spot：%3")
                      : tr("预览能层序号：%1 / %2    本层 Spot：%3"))
                     .arg(displayedLayer)
                     .arg(m_beams.at(m_beamIndex).layers.size())
                     .arg(layer.spots.size());
    }
    painter.drawText(QRect(12, height() - 30, width() - 24, 20),
                     Qt::AlignLeft | Qt::AlignVCenter, footer);
}

void SpotMapWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_cachedPlotSize != plotRect().size()) {
        invalidatePlanCache();
    }
    if (m_cachedPointCloudSize != plotRect().size()) {
        invalidatePointCloudCache();
    }
}

void SpotMapWidget::mousePressEvent(QMouseEvent *event)
{
    if (m_viewMode == ViewMode::Plan2D && event->button() == Qt::LeftButton
        && plotRect().contains(event->position().toPoint())) {
        m_panning = true;
        m_lastMousePosition = event->position().toPoint();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    if (mode2DRect().contains(event->position().toPoint())) {
        m_viewMode = ViewMode::Plan2D;
        update();
        return;
    }
    if (mode3DRect().contains(event->position().toPoint())) {
        m_viewMode = ViewMode::PointCloud3D;
        invalidatePointCloudCache();
        update();
        return;
    }
    if (m_viewMode == ViewMode::PointCloud3D
        && topViewRect().contains(event->position().toPoint())) {
        m_yawDegrees = 0.0;
        m_pitchDegrees = 0.0;
        invalidatePointCloudCache();
        update();
        return;
    }
    if (m_viewMode == ViewMode::PointCloud3D
        && sideViewRect().contains(event->position().toPoint())) {
        m_yawDegrees = 0.0;
        m_pitchDegrees = 90.0;
        invalidatePointCloudCache();
        update();
        return;
    }
    if (m_viewMode == ViewMode::PointCloud3D
        && event->button() == Qt::LeftButton
        && plotRect().contains(event->position().toPoint())) {
        m_rotating = true;
        m_lastMousePosition = event->position().toPoint();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void SpotMapWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_viewMode == ViewMode::Plan2D) {
        if (m_panning && !(event->buttons() & Qt::LeftButton)) {
            m_panning = false;
            unsetCursor();
        }
        if (m_panning) {
            m_planPan += event->position().toPoint() - m_lastMousePosition;
            m_lastMousePosition = event->position().toPoint();
            invalidatePlanCache();
        } else {
            m_hoveredSpot = spotIndexAt(event->position().toPoint());
            const QString info = spotInfoAt(event->position().toPoint());
            if (info.isEmpty()) QToolTip::hideText();
            else QToolTip::showText(event->globalPosition().toPoint(), info, this);
        }
        update();
        event->accept();
        return;
    }
    if (!m_rotating || m_viewMode != ViewMode::PointCloud3D) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    const QPoint currentPosition = event->position().toPoint();
    const QPoint delta = currentPosition - m_lastMousePosition;
    m_lastMousePosition = currentPosition;
    m_yawDegrees += delta.x() * 0.45;
    m_pitchDegrees = qBound(-90.0,
                            m_pitchDegrees + delta.y() * 0.35,
                            90.0);
    invalidatePointCloudCache();
    update();
    event->accept();
}

void SpotMapWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_panning && event->button() == Qt::LeftButton) {
        m_panning = false;
        unsetCursor();
        event->accept();
        return;
    }
    if (m_rotating && event->button() == Qt::LeftButton) {
        m_rotating = false;
        unsetCursor();
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void SpotMapWidget::wheelEvent(QWheelEvent *event)
{
    if (m_viewMode == ViewMode::Plan2D && plotRect().contains(event->position().toPoint())
        && event->angleDelta().y() != 0) {
        const double previous = m_planZoom;
        m_planZoom = qBound(0.5, m_planZoom * std::pow(1.0015, event->angleDelta().y()), 8.0);
        const QPointF relative = event->position() - plotRect().center();
        m_planPan = relative - (relative - m_planPan) * (m_planZoom / previous);
        m_hoveredSpot = -1;
        QToolTip::hideText();
        invalidatePlanCache();
        update();
        event->accept();
        return;
    }
    if (m_viewMode != ViewMode::PointCloud3D
        || !plotRect().contains(event->position().toPoint())) {
        QWidget::wheelEvent(event);
        return;
    }

    const double zoomFactor = event->angleDelta().y() > 0 ? 1.12 : 0.89;
    m_zoom = qBound(0.55, m_zoom * zoomFactor, 2.4);
    invalidatePointCloudCache();
    update();
    event->accept();
}

void SpotMapWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && plotRect().contains(event->position().toPoint())) {
        m_planZoom = 1.0;
        m_planPan = QPointF();
        m_zoom = 1.0;
        m_panning = false;
        m_rotating = false;
        unsetCursor();
        invalidatePlanCache();
        invalidatePointCloudCache();
        update();
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void SpotMapWidget::leaveEvent(QEvent *event)
{
    m_hoveredSpot = -1;
    QToolTip::hideText();
    update();
    QWidget::leaveEvent(event);
}
