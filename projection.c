#include <cairo.h>
#include <cairo-pdf.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>


#define max(a,b) ((a) > (b)) ? (a) : (b)
#define min(a,b) ((a) < (b)) ? (a) : (b)

#define PI 3.141592653589793238462643383
#define ratio PI/180.
#define POINT_UNDEFINED -99999.

/* Modify these */
#define lat_0 60*ratio
#define lon_0 0*ratio
#define nb_lon 20
#define nb_lat 10

#define radius 10
#define width 500
#define height 500
#define x_center width/2
#define y_center height/2
#define nb_points 20
#define scale 20.

#define fill_cell 1

int is_visible(float lat, float lon)
{
  float dist; 
  float prec = 1e-4; 
  dist = sin(lat_0)*sin(lat*ratio) + cos(lat_0)*cos(lat*ratio)*cos(lon*ratio - lon_0);

  return (dist > -prec);
}

/* Input in degrees. Assumes the point is visible */
int map_to_orthographic(float* x, float* y, float lat, float lon)
{
  float lat1 = lat*ratio;
  float lon1 = lon*ratio;

  (*x) = radius*cos(lat1)*sin(lon1 - lon_0);
  (*y) = cos(lat_0)*sin(lat1) - sin(lat_0)*cos(lat1)*cos(lon1 - lon_0);
  (*y) *= radius;

  return 1;
}

/* Draw the visible part of an arc */
void draw_arc(float start[], float end[], cairo_t* cr) 
{
  int i;
  int res1, res2;
  float dlat, dlon;
  float x1, y1;
  float x2, y2;

  dlat = (end[0] - start[0]) / nb_points;
  dlon = (end[1] - start[1]) / nb_points;
  //  printf("dlat dlon %f %f \n", dlat, dlon);
  for (i = 0; i < nb_points; ++i)
  {
    res1 = map_to_orthographic(&x1, &y1, start[0] + i*dlat, start[1] + i*dlon);
    res2 = map_to_orthographic(&x2, &y2, start[0] + (i+1)*dlat, start[1] + (i+1)*dlon);

    /* No need for that anymore */
    if (res1 > 0 && res2 > 0) {
      /* Beware, y=0 is top of the figure, so we must revert */
      if (fill_cell) {
        cairo_line_to(cr, x_center + x2*scale, height-(y_center + y2*scale));
        cairo_stroke_preserve(cr);
      }
      else {
        cairo_move_to(cr, x_center + x1*scale, height-(y_center + y1*scale));
        cairo_line_to(cr, x_center + x2*scale, height-(y_center + y2*scale));
        cairo_stroke(cr);
      }

    }
  }
}

/* Find first visible point between (lat0, lon0) and (lat1, lon1)
 * Assume (lat0, lon0) < (lat1, lon1) */
float find_min_visible(float lon0, float lon1, float lat) {
  float dlon, cur;
  float first_lon, last_lon;
  int i;

  first_lon = lon0;//min(lon0, lon1);
  last_lon = lon1;//max(lon0, lon1);
  dlon = (last_lon - first_lon) / nb_points;
  for (i = 0; i < nb_points+1; ++i)
  {
    cur = first_lon + i*dlon;
    if (is_visible(lat, cur)) return cur;
  }
  return 0.;
}

/* Find last visible point on the segment defined by its extremities 
 * end_n contains the new value (lat, lon)
 * Assume (lat0, lon0) < (lat1, lon1) */
void find_last_visible(float start[], float end[], float end_n[]) {
  float dlon, dlat;
  float cur[2];
  int i;

  dlat = (end[0] - start[0]) / nb_points;
  dlon = (end[1] - start[1]) / nb_points;
  for (i = 0; i < nb_points+1; ++i)
  {
    cur[0] = end[0] - i*dlat;
    cur[1] = end[1] - i*dlon;
    if (is_visible(cur[0], cur[1])) {
      end_n[0] = cur[0];
      end_n[1] = cur[1];
      return;
    }
  }
}

/* Find first visible point on the segment defined by its extremities 
 * end contains the new value (lat, lon)
 * Assume (lat0, lon0) < (lat1, lon1) */
void find_first_visible(float start[], float end[], float start_n[]) {
  float dlon, dlat;
  float cur[2];
  int i;

  dlat = (end[0] - start[0]) / nb_points;
  dlon = (end[1] - start[1]) / nb_points;
  for (i = 0; i < nb_points+1; ++i)
  {
    cur[0] = start[0] + i*dlat;
    cur[1] = start[1] + i*dlon;
    if (is_visible(cur[0], cur[1])) {
      start_n[0] = cur[0];
      start_n[1] = cur[1];
      return;
    }
  }
}

/* We have all the sides of the cells, twice. We remove the invisible parts
 * according to an array 
 * The result is in a_new */
void generate_cell(float a_new[], float a_new2[], float b_new[], 
    float b_new2[], float c_new[], float c_new2[], float d_new[], 
    float d_new2[], int res[]){
  int i;

  if (res[0] == -1) {
    for (i = 0; i < 2; ++i) {
      a_new[i] = a_new2[i];
      b_new[i] = b_new2[i];
    }
  }
  /* Do nothing 
     else if (res[1] == -1) { */
  else if (res[2] == -1) {
    for (i = 0; i < 2; ++i) {
      c_new[i] = c_new2[i];
      d_new[i] = d_new2[i];
    }
  }
/* Do nothing 
 * else  */
}



/* Replace the segment extremities with the visible extremities
 * start_n, end_n contains the new segment
 * Returns 1 if we modified the segment, -1 if the segment is invisible,
 * 0 otherwise */
int set_to_visible(float start[], float end[], float start_n[], float end_n[]) {
  float min_lon, max_lon;
  int x, res;
  int i;

  /* Default is equal */
  for (i = 0; i < 2; i++) {
    start_n[i] = start[i];
    end_n[i] = end[i];
  }

  if (is_visible(start[0], start[1])) {
    if (is_visible(end[0], end[1])) {
      return 0;
    }
    else {
      //printf("last not\n");
      find_last_visible(start, end, end_n);
      return 1;
    }
  }
  else {
    if (is_visible(end[0], end[1])) {
      //!printf("first not visible for %f %f\n", start[0], start[1]);
      //printf("start end %f %f %f %f\n", start[0], start[1], end[0], end[1]);
      find_first_visible(start, end, start_n);
//      printf("first not\n");
//      printf("start %f %f\n", start[0], start[1]);
//      printf("start_new %f %f\n", start_n[0], start_n[1]);
      return 1;
      //printf("start end %f %f %f %f\n", start[0], start[1], end[0], end[1]);
    }
    else {
      //printf("all invisible\n");
      /*start_n[0] = POINT_UNDEFINED;
        start_n[1] = POINT_UNDEFINED;
        end_n[0] = POINT_UNDEFINED;
        end_n[1] = POINT_UNDEFINED;*/
      return -1;
    }
  }
}

/* (lat, lon) is upper left corner and in degree */
/* dlat > 0 */
void draw_cell(float lat, float lon, float dlat, float dlon, cairo_t *cr){
  float x1, y1;
  float x2, y2;
  float a[2] = {lat, lon};
  float b[2] = {lat, lon+dlon};
  float c[2] = {lat-dlat, lon+dlon};
  float d[2] = {lat-dlat, lon};
  float a_new[2], b_new[2], c_new[2], d_new[2];
  float a_new2[2], b_new2[2], c_new2[2], d_new2[2];
  float r1, g1, b1;
  int cond, i;
  int res[4];
  int canDraw, count;

  res[0] = set_to_visible(a, b, a_new, b_new);
  res[1] = set_to_visible(b, c, b_new2, c_new2);
  res[2] = set_to_visible(c, d, c_new, d_new);
  //  for (i = 0; i < 2; ++i) 
  //     printf("cd %f %f\n", c[i], d[i]);
  //  for (i = 0; i < 2; ++i) 
  //     printf("cnew dnew %f %f\n", c_new[i], d_new[i]);

  res[3] = set_to_visible(d, a, d_new2, a_new2);
  //printf("res %d %d %d %d \n", res[0], res[1], res[2], res[3]);
  //canDraw = res[0] != -1 && res[1] != -1  && res[2] != -1 && res[3] != -1;
  count = 0;
  for (i = 0; i < 4; ++i) {
    if (res[i] > -1) count++;
  }
  canDraw = count > 2;

  /* All the cell must be visible as we corrected the extremities */
  if (canDraw) {
    generate_cell(a_new, a_new2, b_new, b_new2, c_new, c_new2, d_new, d_new2, res);
    /* for (i = 0; i < 2; ++i) 
       printf("ab %f %f\n", a_new[i], b_new[i]);
       for (i = 0; i < 2; ++i) 
       printf("bc %f %f\n", b_new[i], c_new[i]);
       for (i = 0; i < 2; ++i) 
       printf("cd %f %f\n", c_new[i], d_new[i]);
       */

    cairo_set_source_rgb(cr, 0., 0., 0.);
    draw_arc(a_new, b_new, cr);
    draw_arc(b_new, c_new, cr);
    draw_arc(c_new, d_new, cr);
    draw_arc(d_new, a_new, cr);
    if (fill_cell) {
      cairo_close_path(cr);
      r1 = (float)rand()/(float)RAND_MAX;
      g1 = (float)rand()/(float)RAND_MAX;
      b1 = (float)rand()/(float)RAND_MAX;
      cairo_set_source_rgb(cr, r1, g1, b1);
      cairo_fill(cr);
    }
  }
}

void draw_outer_circle(cairo_t *cr)
{
  cairo_set_source_rgb (cr, 0, 1, 0);
  cairo_arc (cr, x_center, y_center, radius*scale, 0, 2*M_PI);
  cairo_stroke(cr);
}

int main (int argc, char *argv[])
{
  FILE *input;
  cairo_surface_t *cs;
  cairo_t *cr;
  float margin = 5.;
  float dlon, dlat;
  float lon;
  int i, j;

  srand(time(NULL));

  /* Create a surface and a cairo_t */
  /* A4 format approx */
  cs = cairo_pdf_surface_create("test.pdf", width, height);
  cr = cairo_create (cs);
  cairo_set_source_rgb(cr,0,0,0);
  cairo_set_line_width(cr,0.5);
  cairo_stroke(cr);

  dlon = 360./nb_lon;
  dlat = 180./nb_lat;
   for (i = 0; i < nb_lat; ++i) {
      for (j = 0; j < nb_lon; ++j) {
  //for (i = 2; i < 3; ++i) {
   //       for (j = 6; j < 7; ++j) {
      draw_cell(90.-i*dlat, j*dlon, dlat, dlon, cr);
    }
    }

    draw_outer_circle(cr);

    /* Needed for PDF output */
    cairo_show_page(cr);
    cairo_destroy(cr);

    cairo_surface_flush(cs);
    cairo_surface_destroy(cs);

    return 0;
  }

