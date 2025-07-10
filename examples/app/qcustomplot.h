#ifndef QCUSTOMPLOT_H
#define QCUSTOMPLOT_H

#include <QWidget>
#include <QObject>
#include <QPen>
#include <QVector>

struct QCPRange {
    double lower, upper;
    QCPRange(double l = 0, double u = 0) : lower(l), upper(u) {}
};

class QCustomPlot;

class QCPAxis : public QObject {
    Q_OBJECT
public:
    enum AxisType { atLeft, atRight, atTop, atBottom };

    QCPAxis(QCustomPlot* parent, AxisType type);
    void setLabel(const QString& str);
    QString label() const;
    void setRange(double lower, double upper);
    QCPRange range() const;

private:
    QCustomPlot* mParent;
    AxisType mAxisType;
    QCPRange mRange;
    QString mLabel;
};

class QCPGraph : public QObject {
    Q_OBJECT
public:
    QCPGraph(QCustomPlot* parent);
    void setData(const QVector<double>& keys, const QVector<double>& values);
    const QVector<double>& keys() const;   // Changed to return const reference
    const QVector<double>& values() const; // Changed to return const reference
    void setPen(const QPen& pen);
    QPen pen() const;
    void setName(const QString& name);
    QString name() const;

private:
    QCustomPlot* mParent;
    QVector<double> mKeys;
    QVector<double> mValues;
    QPen mPen;
    QString mName;
};

class QCPLegend : public QObject {
    Q_OBJECT
public:
    QCPLegend(QCustomPlot* parent);
    void setVisible(bool visible);
    bool visible() const;

private:
    QCustomPlot* mParent;
    bool mVisible;
};

class QCustomPlot : public QWidget {
    Q_OBJECT
public:
    QCustomPlot(QWidget* parent = nullptr);

    QCPGraph* addGraph();
    QCPGraph* graph(int index) const;
    void replot();

    QCPAxis* xAxis;
    QCPAxis* yAxis;
    QCPLegend* legend;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector<QCPGraph*> mGraphs;
};

#endif // QCUSTOMPLOT_H