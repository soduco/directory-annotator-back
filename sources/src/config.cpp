#include "config.hpp"
#include <cmath>

int kDebugLevel = 0;


float KConfig::kLineHorizontalSigma = 10;
float KConfig::kLineVerticalSigma   = 3;
float KConfig::kAngleTolerance      = 5.f;
int   KConfig::kLayoutPageOpeningWidth  = 200 * 2;
int   KConfig::kLayoutPageOpeningHeight = 200 * 2;
float KConfig::kLayoutPageFullLineWhite = 0.95f;
float KConfig::kLayoutPageMargin = 0.1f;
int   KConfig::kLayoutWhiteLevel = 150;
float KConfig::kLayoutBlockFillingRatio = 0.5f;
float KConfig::kPourcentEmptyColumn = 0.1f;
float KConfig::kPourcentEmptyLine = 0.1f;

KConfig::KConfig(int xheight, int scale)
{
  if (scale == 0)
    xheight /= 2;

  kxheight     = xheight;
  kOneEm       = 1.5 * xheight; // Conversion pt -> pixel
  kLineHeight  = 2.0 * xheight;
  kWordSpacing = 0.5 * xheight;
  kWordWidth   = 4.0 * xheight;

  kLayoutBlockOpeningWidth  = int(std::round(kOneEm));
  kLayoutBlockOpeningHeight = int(std::round(xheight));
  kLayoutBlockMinHeight     = 0.5  * xheight + .5f;
  kLayoutBlockMinWidth      = 10.0 * xheight + .5f;

  kColumnMinSpacing = int(std::round(1 * kOneEm));
  kColumnMinSize = int(std::round(6 * kOneEm));


  kCountEmptyLine = std::round(0.5f * kOneEm);


  //kSectionMinSpacing = int(0);
  kSectionMinSpacing = int(std::round(0.5f * xheight));
  kSectionMinSize = int(std::round(xheight));
}


