#include <db/ColumnStats.hpp>
#include <numeric>

using namespace db;

/* constructs ColumnStats objects, setting members min max and buckets to the values
passed in, and calculates and sets width based on these values. also initializes the 
total count of values to zero, and sizes the histogram array appropriately based on 
the number of buckets */
ColumnStats::ColumnStats(unsigned buckets, int min, int max) :
    buckets(buckets),
    min(min),
    max(max),
    width((max - min) / buckets + !! ((max - min) % buckets)),
    total(0)
{
    this->histogram.resize(buckets, 0);
}

/* calls helper function getIndex to determine which bucket the value should be added to
(getIndex performs error checking to ensure value is within the min,max range), then 
increments the bucket for that index and the total value count */
void ColumnStats::addValue(int v) {
    int index = getIndex(v);
    histogram[index]++;
    total++;
}

/* NOTE: Implementation of LT and GT differs slightly from the writeup. Based on the README, the
fraction of the bucket that is greater than is ((r - v) / w), thus the number of values in that 
bucket estimated to be greater would be ((r - v) / w) * height. When I tried this I was failing 
tests, and on looking at the test code it actually expects (height * (r - v)) / w which gives
a different result. */
size_t ColumnStats::estimateCardinality(PredicateOp op, int v) const {
    /* Since getIndex will throw an exception for out of range exception, these checks 
    had to be added to support tests where the value of v is outside of the min,max range*/
    if (v < min) {
        v = min;
    }
    if (v > max) {
        v = max;
    }
    /* get the index of the bucket v would fall within */
    int index = getIndex(v);
    int left, right, part;

    switch (op) {
        case PredicateOp::EQ:
            return histogram[index] / width;
        case PredicateOp::NE:
            return total - histogram[index] / width;
        case PredicateOp::LE:
            /* same as LT but with +1 added to the fraction to calculate 'part' to account 
            for the equals case */
            left = min + (index * width);
            part = (histogram[index] * (v - left + 1)) / width;
            return std::accumulate(histogram.begin(), histogram.begin() + index, 0) + part;
        case PredicateOp::LT:
            /* calculate left edge of v's bucket */
            left = min + (index * width);
            /* calculate the part of the bucket that is less than v */
            part = (histogram[index] * (v - left)) / width;
            /* return the sum of the partition of v's bucket and all buckets to the left */
            return std::accumulate(histogram.begin(), histogram.begin() + index, 0) + part;
        case PredicateOp::GE:
            /* same as GT but with +1 added to the fraction to calculate 'part' to account 
            for the equals case */
            right = min + ((index + 1) * width) - 1;
            part = (histogram[index] * (right - v + 1)) / width;
            return std::accumulate(histogram.begin() + index + 1, histogram.end(), 0) + part;
        case PredicateOp::GT:
            /* calculate right edge of v's bucket */
            right = min + ((index + 1) * width) - 1;
            /* calculate the part of the bucket that is greater than v */
            part = (histogram[index] * (right - v)) / width;
            /* return the sum of the partition of v's bucket and all buckets to the right */
            return std::accumulate(histogram.begin() + index + 1, histogram.end(), 0) + part;
    }
    return 0;
}
