#include "header.h"

int collide2d(float x1, float y1, float x2, float y2, float wt1, float ht1, float wt2, float ht2)
{
    if (!isfinite(x1) || !isfinite(y1) || !isfinite(x2) || !isfinite(y2) ||
        !isfinite(wt1) || !isfinite(ht1) || !isfinite(wt2) || !isfinite(ht2) ||
        wt1 <= 0.0f || ht1 <= 0.0f || wt2 <= 0.0f || ht2 <= 0.0f)
    {
        return 0;
    }
    const double firstRight = (double)x1 + (double)wt1;
    const double firstBottom = (double)y1 + (double)ht1;
    const double secondRight = (double)x2 + (double)wt2;
    const double secondBottom = (double)y2 + (double)ht2;
    return (double)x1 < secondRight && (double)x2 < firstRight &&
           (double)y1 < secondBottom && (double)y2 < firstBottom;
}
