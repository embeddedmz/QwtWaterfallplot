#ifndef WATERFALLDATA_H
#define WATERFALLDATA_H

#include <qwt_matrix_raster_data.h>

#include <ctime>

template <class T>
class WaterfallData : public QwtMatrixRasterData
{
    static_assert(std::is_arithmetic<T>::value, "WaterfallData's data must be numeric !");

public:
    WaterfallData(double dXMin, double dXMax, // X bounds
                  const size_t historyExtent, // will define Y width
                  const size_t layerPoints) :
        m_data(new T[historyExtent * layerPoints]),
        m_layerPoints(layerPoints),
        m_maxHistoryLength(historyExtent),
        m_currentHistoryLength(0),
        m_layersTimestamps(new time_t[historyExtent])
    {
        if (m_layerPoints == 0 || m_maxHistoryLength == 0)
        {
            throw "Bad usage of WaterfallData !"; // better: call abort();
        }

        // initialize data with zeroes or the minimal value of T type
        clear();

        // sanitize
        if (dXMin > dXMax)
        {
            std::swap(dXMin, dXMax);
        }

        setInterval(Qt::XAxis,
                    QwtInterval(dXMin, dXMax, QwtInterval::ExcludeMaximum));
        setInterval(Qt::YAxis,
                    QwtInterval(0, m_maxHistoryLength, QwtInterval::ExcludeMaximum));
    }

    ~WaterfallData() override
    {
        delete [] m_data;
        delete [] m_layersTimestamps;
    }

    // overriden methods
    double value(double x, double y) const override
    {
        const QwtInterval xInterval = interval(Qt::XAxis);
        const QwtInterval yInterval = interval(Qt::YAxis);

        // + valid
        if (!(xInterval.contains(x) && yInterval.contains(y)))
        {
            return qQNaN();
        }

        // spacing !
        double dx = xInterval.width() / m_layerPoints;
        double dy = yInterval.width() / m_maxHistoryLength;

        int row = int((y - yInterval.minValue()) / dy);
        int col = int((x - xInterval.minValue()) / dx);

        if (row >= m_maxHistoryLength)
        {
            row = m_maxHistoryLength - 1;
        }
        if (col >= m_layerPoints)
        {
            col = m_layerPoints - 1;
        }

        return double(m_data[row * m_layerPoints + col]);
    }

    /* pixelHint() returns the geometry of a pixel, that can be used
       to calculate the resolution and alignment of the plot item, that is
       representing the data.

       - NearestNeighbour\n
         pixelHint() returns the surrounding pixel of the top left value
         in the matrix.

       - BilinearInterpolation\n
         Returns an empty rectangle recommending
         to render in target device ( f.e. screen ) resolution.
    */
    QRectF pixelHint(const QRectF& area) const override
    {
        Q_UNUSED(area)

        QRectF rect;
        if (resampleMode() == NearestNeighbour)
        {
            const QwtInterval intervalX = interval(Qt::XAxis);
            const QwtInterval intervalY = interval(Qt::YAxis);
            if (intervalX.isValid() && intervalY.isValid())
            {
                // spacing in X and Y
                const double dx = intervalX.width() / m_layerPoints;
                const double dy = intervalY.width() / m_maxHistoryLength;

                rect = QRectF(intervalX.minValue(), intervalY.minValue(),
                              dx, dy);
            }
        }
        return rect;
    }

    bool addData(const T* const fftData, const size_t length)
    {
        if (length != m_layerPoints)
        {
            return false;
        }

        // another solution is to use move_backward and use m_currentHistoryLength
        // the only benefit is to move only the filled layers
        // and not all the waterfall layers - 1 when this last is not completely filled !
        std::move(m_data + m_layerPoints,
                  m_data + m_layerPoints + (m_maxHistoryLength - 1) * m_layerPoints,
                  m_data);

        std::copy(fftData, fftData + length, &m_data[m_layerPoints * (m_maxHistoryLength - 1)]);

        // do the same for the array of timestamps !
        std::move(m_layersTimestamps + 1,
                  m_layersTimestamps + 1 + (m_maxHistoryLength - 1),
                  m_layersTimestamps);

        time(&m_layersTimestamps[m_maxHistoryLength - 1]);

        if (m_currentHistoryLength < m_maxHistoryLength)
        {
            ++m_currentHistoryLength;
        }

        return true;
    }

    void clear()
    {
        std::fill(m_data, m_data + m_layerPoints * m_maxHistoryLength, 0.);
        m_currentHistoryLength = 0;

        std::fill(m_layersTimestamps, m_layersTimestamps + m_maxHistoryLength, 0);
    }

    inline size_t getLayerPoints() const { return m_layerPoints; }
    inline size_t getMaxHistoryLength() const { return m_maxHistoryLength; }
    inline size_t getHistoryLength() const { return m_currentHistoryLength; }

    // representation/view data range (may not be equal to the stored data range)
    void setRange(double dLower, double dUpper)
    {
        if (dLower > dUpper)
        {
            std::swap(dLower, dUpper);
        }
        setInterval(Qt::ZAxis, QwtInterval(dLower, dUpper));
    }

    void getRange(double& rangeMin, double& rangeMax) const
    {
        const QwtInterval& range = interval(Qt::ZAxis);
        rangeMin = range.minValue();
        rangeMax = range.maxValue();
    }

    // stored data range !
    void getDataRange(double& rangeMin, double& rangeMax) const
    {
        if (m_currentHistoryLength > 0)
        {
            auto resultPair = std::minmax_element(
                m_data + (m_maxHistoryLength - m_currentHistoryLength) * m_layerPoints,
                m_data + m_layerPoints * m_maxHistoryLength);
            rangeMin = double(*resultPair.first);
            rangeMax = double(*resultPair.second);
        }
        else
        {
            rangeMin = rangeMax = 0;
        }
    }

    size_t getCurrentHistoryLength() const { return m_currentHistoryLength; }

    time_t getLayerDate(const double y) const
    {
        const size_t index = y;
        if (index < m_maxHistoryLength)
        {
            return m_layersTimestamps[index];
        }
        return 0;
    }

    const T* getData() const { return m_data; }

protected:
    T* const     m_data;
    const size_t m_layerPoints;          // fft points
    const size_t m_maxHistoryLength;     // max number of layers (Y width)
    size_t       m_currentHistoryLength; // filled layers count

    time_t* const m_layersTimestamps;
};

#endif // WATERFALLDATA_H
