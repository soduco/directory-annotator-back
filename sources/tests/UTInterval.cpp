#include <gtest/gtest.h>
#include "../src/Interval.hpp"


TEST(UTInterval, Merge) {
  IntervalSet s;
  s.insert(2, 5);
  s.insert(8, 15);
  s.insert(5, 8);

  auto r = s.intervals();
  ASSERT_EQ(r.size(), 1);
  ASSERT_EQ(r[0], (Interval{2, 15}));
}

TEST(UTInterval, InsertFirst) {
  IntervalSet s;
  s.insert(2, 5);
  s.insert(8, 15);
  s.insert(-1, 1);

  auto r = s.intervals();
  ASSERT_EQ(r.size(), 3);
  ASSERT_EQ(r[0], (Interval{-1, 1}));
  ASSERT_EQ(r[1], (Interval{2, 5}));
}

TEST(UTInterval, InsertLast) {
  IntervalSet s;
  s.insert(2, 5);
  s.insert(8, 15);
  s.insert(18, 21);

  auto r = s.intervals();
  ASSERT_EQ(r.size(), 3);
  ASSERT_EQ(r[1], (Interval{8, 15}));
  ASSERT_EQ(r[2], (Interval{18, 21}));
}

TEST(UTInterval, InsertMiddle) {
  IntervalSet s;
  s.insert(2, 5);
  s.insert(8, 15);
  s.insert(6, 7);

  auto r = s.intervals();
  ASSERT_EQ(r.size(), 3);
  ASSERT_EQ(r[0], (Interval{2, 5}));
  ASSERT_EQ(r[1], (Interval{6, 7}));
  ASSERT_EQ(r[2], (Interval{8, 15}));
}


TEST(UTInterval, MergeLeft) {
  IntervalSet s;
  s.insert(2, 5);
  s.insert(8, 15);
  s.insert(4, 7);

  auto r = s.intervals();
  ASSERT_EQ(r.size(), 2);
  ASSERT_EQ(r[0], (Interval{2, 7}));
  ASSERT_EQ(r[1], (Interval{8, 15}));
}


TEST(UTInterval, MergeRight) {
  IntervalSet s;
  s.insert(2, 5);
  s.insert(8, 15);
  s.insert(6, 17);

  auto r = s.intervals();
  ASSERT_EQ(r.size(), 2);
  ASSERT_EQ(r[0], (Interval{2, 5}));
  ASSERT_EQ(r[1], (Interval{6, 17}));
}