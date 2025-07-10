#include "qcustomplot.h"
#include <QPainter>
#include <QPaintEvent>
#include <QDebug>
#include <QPainterPath>

QCPAxis::QCPAxis(QCustomPlot* parent, AxisType type)
    : QObject(parent), mParent(parent), mAxisType(type), mRange(0, 0), mLabel("") {}

void QCPAxis::setLabel(const QString& str) { mLabel = str; }
QString QCPAxis::label() const { return mLabel; }

void QCPAxis::setRange(double lower, double upper) { mRange = QCPRange(lower, upper); }
QCPRange QCPAxis::range() const { return mRange; }

QCPGraph::QCPGraph(QCustomPlot* parent)
    : QObject(parent), mParent(parent) {}

void QCPGraph::setData(const QVector<double>& keys, const QVector<double>& values) {
    mKeys = keys;
    mValues = values;
}

const QVector<double>& QCPGraph::keys() const { return mKeys; }
const QVector<double>& QCPGraph::values() const { return mValues; }

void QCPGraph::setPen(const QPen& pen) { mPen = pen; }
QPen QCPGraph::pen() const { return mPen; }

void QCPGraph::setName(const QString& name) { mName = name; }
QString QCPGraph::name() const { return mName; }

QCPLegend::QCPLegend(QCustomPlot* parent)
    : QObject(parent), mParent(parent), mVisible(false) {}

void QCPLegend::setVisible(bool visible) { mVisible = visible; }
bool QCPLegend::visible() const { return mVisible; }

QCustomPlot::QCustomPlot(QWidget* parent)
    : QWidget(parent) {
    xAxis = new QCPAxis(this, QCPAxis::atBottom);
    yAxis = new QCPAxis(this, QCPAxis::atLeft);
    legend = new QCPLegend(this);
    setMinimumSize(300, 200);
}

QCPGraph* QCustomPlot::addGraph() {
    QCPGraph* graph = new QCPGraph(this);
    mGraphs.append(graph);
    return graph;
}

QCPGraph* QCustomPlot::graph(int index) const {
    return index < mGraphs.size() ? mGraphs[index] : nullptr;
}

void QCustomPlot::replot() {
    update();
}

void QCustomPlot::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw background
    painter.fillRect(rect(), Qt::white);

    // Calculate plot area with adjusted margins
    QRectF plotArea(30, 10, width() - 40, height() - 30);

    // Draw axes
    painter.setPen(Qt::black);
    painter.drawLine(plotArea.bottomLeft(), plotArea.bottomRight());
    painter.drawLine(plotArea.bottomLeft(), plotArea.topLeft());

    // Draw axis labels
    if (!xAxis->label().isEmpty()) {
        painter.drawText(plotArea.bottomLeft() + QPointF((plotArea.width() - painter.fontMetrics().horizontalAdvance(xAxis->label())) / 2, 25),
                         xAxis->label());
    }
    if (!yAxis->label().isEmpty()) {
        QTransform transform;
        transform.translate(10, plotArea.height() / 2);
        transform.rotate(-90);
        painter.setTransform(transform);
        painter.drawText(0, 0, yAxis->label());
        painter.resetTransform();
    }

    // Draw graphs
    for (QCPGraph* graph : mGraphs) {
        const QVector<double>& keys = graph->keys();
        const QVector<double>& values = graph->values();
        if (keys.isEmpty() || values.size() != keys.size()) {
            qDebug() << "[DEBUG] Skipping graph due to empty or mismatched data";
            continue;
        }

        painter.setPen(graph->pen());
        QPainterPath path;
        for (int i = 0; i < keys.size(); ++i) {
            double x = plotArea.left() + (keys[i] - xAxis->range().lower) / (xAxis->range().upper - xAxis->range().lower) * plotArea.width();
            double y = plotArea.bottom() - (values[i] - yAxis->range().lower) / (yAxis->range().upper - yAxis->range().lower) * plotArea.height();
            if (i == 0) {
                path.moveTo(x, y);
            } else {
                path.lineTo(x, y);
            }
        }
        painter.drawPath(path);
    }

    // Draw legend if visible
    if (legend->visible()) {
        int y = 20;
        for (QCPGraph* graph : mGraphs) {
            if (graph->name().isEmpty()) continue;
            painter.setPen(graph->pen());
            painter.drawLine(width() - 100, y, width() - 80, y);
            painter.setPen(Qt::black);
            painter.drawText(width() - 75, y + 5, graph->name());
            y += 20;
        }
    }
}