#ifndef SPOTMAPWIDGET_H
#define SPOTMAPWIDGET_H

#include <QJsonObject>
#include <QPixmap>
#include <QPointF>
#include <QVector>
#include <QWidget>

class QMouseEvent;
class QWheelEvent;

class SpotMapWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SpotMapWidget(QWidget *parent = nullptr);

    void setPlanPayload(const QJsonObject &payload);
    void setTelemetry(const QJsonObject &payload);
    void clearPlan();
    QString spotInfoAt(const QPoint &position) const;

    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    enum class ViewMode {
        Plan2D,
        PointCloud3D
    };

    struct SpotPoint {
        QPointF position;
        double weight = 0.0;
        int sourceIndex = 0;
    };

    struct LayerData {
        int index = 0;
        double energyMev = -1.0;
        QVector<SpotPoint> spots;
    };

    struct BeamData {
        int number = 0;
        QString name;
        int declaredLayerCount = 0;
        int declaredSpotCount = 0;
        QVector<LayerData> layers;
    };

    QJsonObject planSummary(const QJsonObject &payload) const;
    int findBeamIndex(int beamNumber) const;
    int findLayerIndex(const BeamData &beam, int layerNumber) const;
    QRect plotRect() const;
    QRect mode2DRect() const;
    QRect mode3DRect() const;
    QRect topViewRect() const;
    QRect sideViewRect() const;
    void invalidatePlanCache();
    void invalidatePointCloudCache();
    void rebuildPlanCache();
    void rebuildPointCloudCache();
    QPointF mapSpotToPlot(const QPointF &point) const;
    int spotIndexAt(const QPoint &position) const;
    QPointF projectPoint(double x, double y, double z,
                         const QRect &target,
                         const QRectF &xyBounds,
                         int layerCount) const;
    QString statusText() const;

    QVector<BeamData> m_beams;
    int m_beamIndex = 0;
    int m_layerIndex = 0;
    int m_currentLayerNumber = -1;
    bool m_isDelivering = false;

    QRectF m_spotBounds;
    QPixmap m_planCache;
    QSize m_cachedPlotSize;
    QPixmap m_pointCloudCache;
    QSize m_cachedPointCloudSize;
    ViewMode m_viewMode = ViewMode::Plan2D;
    QPoint m_lastMousePosition;
    bool m_rotating = false;
    double m_yawDegrees = -35.0;
    double m_pitchDegrees = 28.0;
    double m_zoom = 1.0;
    double m_planZoom = 1.0;
    QPointF m_planPan;
    bool m_panning = false;
    int m_hoveredSpot = -1;
};

#endif // SPOTMAPWIDGET_H
