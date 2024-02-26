#pragma once

extern int kDebugLevel;


struct KConfig
{
  /// Constants for text blocks in mm
  /// \{
  // Number of pixels between two consecutive baselines
  float kLineHeight;

  // Number of blank pixels (spacing) between two words
  float kWordSpacing;

  // Number of approximative pixels for the word "Word"
  float kWordWidth;
  float kxheight;
  float kOneEm;

  // Column-split related parameters
  int kColumnMinSpacing;
  int kColumnMinSize;
  static float kPourcentEmptyColumn; /// Pourcent of black so that the column is considered empty 

  // Section-split related parameters
  int kSectionMinSpacing;
  int kSectionMinSize;
  int kCountEmptyLine;
  static float kPourcentEmptyLine; /// Pourcent of black so that the line is considered empty
  // \}

  static float kLineHorizontalSigma;
  static float kLineVerticalSigma;
  static float kAngleTolerance;    // Maximal deviation in degree that makes consider a line as a vertical line

  static int   kLayoutPageOpeningWidth;
  static int   kLayoutPageOpeningHeight;
  static float kLayoutPageFullLineWhite; // Number of white pixels for which we consider a line/column to be white
  static float kLayoutPageMargin;        // 1/kBorderPageRatio Maximal crop size


  static int kLayoutWhiteLevel;

  static float kLayoutBlockFillingRatio;
  int   kLayoutBlockOpeningWidth;
  int   kLayoutBlockOpeningHeight;
  int   kLayoutBlockMinHeight;
  int   kLayoutBlockMinWidth;

  KConfig() = delete;
  KConfig(int xheight, int scale);
};
