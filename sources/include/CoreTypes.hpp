#pragma once

struct KConfig;


struct Point2D
{
  int x, y;
};

struct Segment
{
  Point2D start, end; // Segment end-points (with start.y <= end.y)
  double width;      // Segment width
  double nfa;        // Segment confidence
  double length;     // Segment length
  double angle;      // Segment angle (in degree between 0 - 180)

  bool is_horizontal(float tolerance = 5.f) const;
  bool is_vertical(float tolerance = 5.f) const;

  int x0() const { return start.x; }
  int y0() const { return start.y; }
  int x1() const { return end.x; }
  int y1() const { return end.y; }
  void set_x0(int v) { start.x = v; }
  void set_y0(int v) { start.y = v; }
  void set_x1(int v) { end.x = v; }
  void set_y1(int v) { end.y = v; }



  void scale(float s);
};


struct Box
{
  int x;
  int y;
  int width;
  int height;

  int x0() const { return x; }
  int y0() const { return y; }
  int x1() const { return x + width; }
  int y1() const { return y + height; }


  void scale(float s);
  void merge(Box other);
  void inflate(int b);
  bool intersects(Box other) const;
  bool empty() const;
  bool has(Point2D p) const;
  bool has(Segment s) const;
  bool is_valid() const noexcept;
};




enum e_force_indent
{
  FORCE_NONE  = 0,
  FORCE_LEFT  = 1,
  FORCE_RIGHT = 2,
};
