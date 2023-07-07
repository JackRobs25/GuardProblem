/* mouse1.c 
Laura Toma

Example using the mouse in OpenGL.  First the mouse is registered via
a callback function. Once registered, this function will be called on
any mouse event in the window.  The user can use this function to
respond to mouse events. This code will print the coordinates of the
point where the mouse is clicked, and will draw a small blue disk at
the point where the mouse is pressed.

*/
#include "geom.h" 

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <float.h>
#include <tuple>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif


#include <vector> 

using namespace std; 



GLfloat red[3] = {1.0, 0.0, 0.0};
GLfloat green[3] = {0.0, 1.0, 0.0};
GLfloat blue[3] = {0.0, 0.0, 1.0};
GLfloat black[3] = {0.0, 0.0, 0.0};
GLfloat white[3] = {1.0, 1.0, 1.0};
GLfloat gray[3] = {0.5, 0.5, 0.5};
GLfloat yellow[3] = {1.0, 1.0, 0.0};
GLfloat magenta[3] = {1.0, 0.0, 1.0};
GLfloat cyan[3] = {0.0, 1.0, 1.0};



/* global variables */
const int WINDOWSIZE = 750; 

//the current polygon 
vector<point2d>  poly;

vector<point2d> guards;

//coordinates of the last mouse click
double mouse_x=-10, mouse_y=-10;  //initialized to a point outside the window

//used to give mouse presses a variety of functions
int poly_init_mode = 0;

int num_vertices = 0;

int num_guards = 0;

// a vector to keep track of the guards' previous moves so that they can 
// continue in that direction if it keeps them inside the polygon
vector<point2d> prev_movs;

// toggled by pressing 'a'
// starts off by default
bool animation_mode = false;

// toggled by pressing 'm'
// when true, a mouse click places down a new guard
// when false, a mouse click moves the guard once
bool additional_guards = true;



/* forward declarations of functions */
void display(void);
void keypress(unsigned char key, int x, int y);
void mousepress(int button, int state, int x, int y);
void timerfunc(); 


bool isVisible(vector<point2d>& poly, int i, point2d p, point2d& intersection);
double point_to_line_dist(point2d p, point2d a, point2d b);
int predecessor(vector<point2d>& poly, int index);
int successor(vector<point2d>& poly, int index);
triangle make_triangle(point2d a, point2d b, point2d c);
vector<triangle> triangulate(vector<point2d>& points, point2d guard);
bool compare_points(point2d a, point2d b);
vector<point2d> visiblePoly (vector<point2d>& poly, point2d guard);
vector<pair<point2d, point2d> > CreateSegmentsFromPolygon(vector<point2d>& poly);
bool polygonSimple(vector<point2d>& poly);
void draw_polygon(vector<point2d>& poly);
void draw_guard(vector<point2d>& guards);
void draw_visible_polygon(vector<point2d>& poly, vector<point2d>& guards);
bool pointInPolygon(vector<point2d>& poly, point2d p);
void move_guards(vector<point2d>& poly, vector<point2d>& guards, vector<point2d>& prev_movs);

/********************************************************/
/* compute the visible polygon of the guard */
/********************************************************/


//return true if vertex poly[i] is visible from point p
bool isVisible(vector<point2d>& poly, int i, point2d p, point2d& intersection) {
  // we want to cast a ray from point p to vertex: polygon[i] and return false if there are any intersections
  // first we can create a line segment from p to polygon[i]
  pair<point2d, point2d> gSegment = make_pair(p, poly[i]);
  // we want to shorten the ray just short of the vertex so that it doesn't confuse that as an intersection
  gSegment.second.x -= (poly[i].x - p.x)/100;
  gSegment.second.y -= (poly[i].y - p.y)/100;

  // next we want to check if the ray intersects any segments of the polygon on its way to poly[i]
  vector<pair<point2d, point2d> > segments = CreateSegmentsFromPolygon(poly);
  char code = '?';
  for (int i = 0; i<segments.size(); i++){
    code = SegSegInt(gSegment.first, gSegment.second, segments[i].first, segments[i].second, intersection);
    if (code == '1'){ // the ray has intersected an edge 
      return false;
    }
  }
  // no intersections so point p is visible from poly[i]
  return true;

}


// returns the distance from point p to the segment ab
double point_to_line_dist(point2d p, point2d a, point2d b) {
  double dx = b.x - a.x;
  double dy = b.y - a.y;
  double num = fabs(dy * p.x - dx * p.y + b.x * a.y - b.y * a.x);
  double den = sqrt(dx * dx + dy * dy);
  return num / den;
}

// returns the index of the predecessor of index i in the vector poly
int predecessor(vector<point2d>& poly, int i){
  if (i == 0){
    return poly.size() - 1;
  }
    return i - 1;
}

// returns the index of the successor of index i in the vector poly
int successor(vector<point2d>& poly, int index){
  if (index == poly.size()-1){
    return 0;
  }
  return index + 1;
}

// initialises a triangle struct used for triangulation of the visible polygon
triangle make_triangle(point2d a, point2d b, point2d c){
  triangle t;
  t.a = a;
  t.b = b;
  t.c = c;
  return t;
}


// Compute the triangulation of a non-convex polygon
// and return a vector of triangles represented as triples of indices
vector<triangle> triangulate(vector<point2d>& points, point2d guard) {
  // the visible polygon is unique in the fact that all vertices are visible from the guard
  // so we can simply construct triangles of consecutive vertices and the guard
  int n = points.size();
  vector<triangle> triangles;
  for (int i = 0; i<points.size(); i++){
    int next = (i + 1) % n;
    triangles.push_back(make_triangle(points[i], points[next], guard));
  }
  return triangles;
}



// compares which point is closer to vertex 0 as you traverse the polygon anticlockwise
// returns true if point a is closer than b and false vice versa
bool compare_points(point2d a, point2d b) {
  // we can compare a and b by assigning a number relative to the index of the vertices they lie between
  vector<pair<point2d, point2d> > segments = CreateSegmentsFromPolygon(poly);
  double position_a = -1;
  double position_b = -1;
  

  // check if a and b are vertices of poly, if they are log their position as the index of that vertex
  for (int i = 0; i<poly.size(); i++){
    if (a.x == poly[i].x && a.y == poly[i].y){
      position_a = i;
    }
    if (b.x == poly[i].x && b.y == poly[i].y){
      position_b = i;
    }
  }
  // if they lie between vertices log their positions as (segment.first + segment.second)/2
  if (position_a == -1){
    for (int i = 0; i < segments.size(); i++){
      if (on_segment(segments[i].first, segments[i].second, a)){
        position_a = (i + (i + 1.0))/2.0;
        break;
      }   
    }
  }
  if (position_b == -1){
    for (int i = 0; i < segments.size(); i++){
      if (on_segment(segments[i].first, segments[i].second, b)){
        position_b = (i + (i + 1.0))/2.0;
        break;
      }   
    } 
  }

  assert(position_a != -1);
  assert(position_b != -1);


  if (position_a < position_b){
    return true;
  }         
  else if (position_b < position_a){
    return false;
  }

  // if both points lie on the same line segment then we want to compare them based on which is closer to segment.first
  // potential seg fault here if we index at something .5 so have to return the half positions to whole numbers
  position_a = (int) position_a;
  position_b = (int) position_b;
  if (distance(a, segments[position_a].first) < distance(b, segments[position_b].first)){ 
    return true;
  }
  return false;
}



// The visible polygon consists of the vertices of the museum polygon that are visible, 
// possibly interleaved with points interior to the edges of the museum that represent 
// intersections with rays from the guard.
// To compute these interior points we want to shoot rays through some vertices of the museum
// the visible polygon returned should be in the correct anticlockwise order since the user should have input the points for the museum
// in anticlockwise order and all the work in this method is done looping through that list of points
// so order should be maintained
vector<point2d> visiblePoly (vector<point2d>& poly, point2d guard){
  //printf("entered visiblePoly\n");
  vector<point2d> vPoly;
  vector<pair<point2d, point2d> > segments = CreateSegmentsFromPolygon(poly);
  point2d intersection;

  // the case for needing to shoot a ray from the guard through a vertex is if it is visible, one of its neighbours is not visible , and its reflex
  for (int i = 0; i < poly.size(); i++) { // for each vertex in the polygon
    point2d nextPt;
    nextPt.x = poly[i].x;
    nextPt.y = poly[i].y;
    if ((isVisible(poly, i, guard, intersection) && !isVisible(poly, predecessor(poly, i), guard, intersection))){
      vPoly.push_back(poly[i]);
      //if i is reflex
      if (!left_on(poly[predecessor(poly,i)],poly[i], poly[successor(poly, i)])){
        // we want to cast a ray from the guard, through i until it hits the polygon and we want to add this to vPoly
        // extend the segment until it hits a different segment or goes out of bounds
        // store the intersection of the extended line segment and the polygon in point p
        point2d p;
        while (true) {
          // extend segment
          nextPt.x += (poly[i].x - guard.x)/10;
          nextPt.y += (poly[i].y - guard.y)/10;
          for (int j = 0; j < segments.size(); j++) {
            // if the segment intersects any edges of the polygon add that intersection to the visible polygon
            if (SegSegInt(guard, nextPt, segments[j].first, segments[j].second, p) == '1') {
              double eps = 1e-6; // set a small tolerance value
              // if the intersection point found is the vertex that the ray is being passed through then we want to ignore it
              if (fabs(p.x - poly[i].x) < eps && fabs(p.y - poly[i].y) < eps) {
                printf("intersection point not valid\n");
                continue;
              }
              else{
                vPoly.push_back(p); // we will sort after so are not worried about order of vPoly at the moment
                break;
              }
            }
          }
          if (!pointInPolygon(poly, nextPt)){
            // if the extension of the line from the guard through the vertex goes outside of the polygon we do not want to consider where it intersects
            break;
          }
        }
      }
    }
    // if the vertex is visible and its successor is NOT visible
    else if ((isVisible(poly, i, guard, intersection) && !isVisible(poly, successor(poly, i), guard, intersection))){
      vPoly.push_back(poly[i]);
      if (!left_on(poly[predecessor(poly,i)],poly[i], poly[successor(poly, i)])){
        // we want to cast a ray from the guard, through i until it hits the polygon and we want to add this to vPoly
        // extend the segment until it hits a different segment or goes out of bounds
        // store the intersection of the extended line segment and the polygon in point p
        point2d p;
        while (true) {
          nextPt.x += (poly[i].x - guard.x)/10;
          nextPt.y += (poly[i].y - guard.y)/10;
          for (int j = 0; j < segments.size(); j++) {
            // if the segment intersects any edges of the polygon add that intersection to the visible polygon
            if (SegSegInt(guard, nextPt, segments[j].first, segments[j].second, p) == '1') {
              double eps = 1e-6; // set a small tolerance value
              // if the intersection point found is the vertex that the ray is being passed through then we want to ignore it
              if (fabs(p.x - poly[i].x) < eps && fabs(p.y - poly[i].y) < eps) {
                continue;
              }
              else{
                vPoly.push_back(p); // we will sort after so are not worried about order of vPoly at the moment
                break;
              }
            }
          }
          if (!pointInPolygon(poly, nextPt)){
            // if the extension of the line from the guard through the vertex goes outside of the polygon we do not want to consider where it intersects
            break;
          }
        }
      }
    }
    // at this point the vertex poly[i] is visible and so is its successor and predecessor
    else if (isVisible(poly, i, guard, intersection)) {
      // add vertex to visible poly
      vPoly.push_back(poly[i]);
    } 
  }

  // now we need to sort vertices in vPoly in an anti-clockwise order relative to the polygon they lie on
  sort(vPoly.begin(), vPoly.end(), compare_points);

  return vPoly;
}
  
      


/********************************************************/
/********************************************************/


// split the points into pairs of points where each pair represents a line segment in a polygon
vector<pair<point2d, point2d> > CreateSegmentsFromPolygon(vector<point2d>& poly) {
  vector<pair<point2d, point2d> > segments;
  for(int i = 0; i < poly.size()-1; i++) {
    segments.push_back(make_pair(poly[i], poly[i+1]));
  }
  segments.push_back(make_pair(poly.back(), poly.front())); // add last segment to close polygon
  return segments;
}

//return true if the polygon is simple, i.e there are no intersecting lines
bool polygonSimple(vector<point2d>& poly){
  //create pairs of line segments
  vector<pair<point2d, point2d> > segments = CreateSegmentsFromPolygon(poly);
  for (int i = 0; i < segments.size(); i++){
    for (int j = i + 1; j < segments.size(); j++){
      point2d p; //point to store intersection returned by SegSegInt
      if (SegSegInt(segments[i].first, segments[i].second, segments[j].first, segments[j].second, p) == '1'){
        return false;
      }
    }
  }
  return true;
}


/* ****************************** */
int main(int argc, char** argv) {

  //printf("press 's' to start entering the vertices of the polygon and 'e' to end. (Please enter the vertices counter-clockwise)\n");

  // initialize GLUT  
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
  glutInitWindowSize(WINDOWSIZE, WINDOWSIZE);
  glutInitWindowPosition(100,100);
  glutCreateWindow(argv[0]);

  // register callback functions 
  glutDisplayFunc(display); 
  glutKeyboardFunc(keypress);
  glutMouseFunc(mousepress); 
  glutIdleFunc(timerfunc); //register this if you want it called at every frame

  // init GL 
  // set background color black
  glClearColor(0, 0, 0, 0);   
  
  // give control to event handler 
  glutMainLoop();

  return 0;
}


/* ****************************** */
/* draw the polygon */
void draw_polygon(vector<point2d>& poly){
   if (poly.size() == 0) return; 

  //set color  and polygon mode 
  glColor3fv(yellow);   
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
 
  int i;
  for (i=0; i<poly.size()-1; i++) {
    //draw a line from point i to i+1
    glBegin(GL_LINES);
    glVertex2f(poly[i].x, poly[i].y); 
    glVertex2f(poly[i+1].x, poly[i+1].y);
    glEnd();
  }
  //draw a line from the last point to the first  
  int last=poly.size()-1; 
    glBegin(GL_LINES);
    glVertex2f(poly[last].x, poly[last].y); 
    glVertex2f(poly[0].x, poly[0].y);
    glEnd();
}

void draw_guard(vector<point2d>& guards){
  if (guards.size() == 0) return;
  glColor3fv(red);   
  glPointSize(5.0f);
  glBegin(GL_POINTS);
  
  int i;
  for (i=0; i<guards.size(); i++) {
    glVertex2f(guards[i].x, guards[i].y); 
  }

  glEnd();
}


void draw_visible_polygon(vector<point2d>& poly, vector<point2d>& guards){

  // Define an array of colors for each guard
  GLfloat colors[][4] = {
    { 1.0f, 0.0f, 0.0f, 0.2f },  // Red
    { 0.0f, 1.0f, 0.0f, 0.2f },  // Green
    { 0.0f, 0.0f, 1.0f, 0.2f },  // Blue
    { 1.0f, 1.0f, 0.0f, 0.2f },  // Yellow
    { 1.0f, 0.0f, 1.0f, 0.2f },  // Magenta
    { 0.0f, 1.0f, 1.0f, 0.2f },  // Cyan
    // Add more colors as needed
  };

  if (guards.size() == 0) return;

  for (int i = 0; i<guards.size(); i++) { // for each guard
    //generate the visible polygon for the guard
    vector<point2d> vPoly = visiblePoly(poly, guards[i]);
    
    if (vPoly.size() == 0) return;
  
    // since openGL can only render convex polygons we need to compute the triangulation of the visible polygon before we can render it
    vector<triangle> visible_triangulation = triangulate(vPoly, guards[i]);
    
    if (visible_triangulation.size() == 0) return;

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Specify polygon color and transparency
    glColor4fv(colors[i % (sizeof(colors) / sizeof(colors[0]))]); // 50% transparency

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Begin rendering the filled polygon
    glBegin(GL_TRIANGLES);

    // loop through each triangulation and render it translucent and filled
    for (int i = 0; i < visible_triangulation.size(); i++){
      glVertex2f(visible_triangulation[i].a.x, visible_triangulation[i].a.y);
      glVertex2f(visible_triangulation[i].b.x, visible_triangulation[i].b.y);
      glVertex2f(visible_triangulation[i].c.x, visible_triangulation[i].c.y);

    }

    // End rendering the polygon
    glEnd();

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Disable blending
    glDisable(GL_BLEND);
  } //for i
}

// check if a point is in the polygon
bool pointInPolygon(vector<point2d>& poly, point2d p){
  int n = poly.size();
  int crossings = 0;
  for (int i = 0; i < n; i++) {
    int j = (i + 1) % n;
    // Check if segment intersects the ray cast right from p
    if (((poly[i].y <= p.y && p.y < poly[j].y) || (poly[j].y <= p.y && p.y < poly[i].y))
        && (poly[i].x > p.x || poly[j].x > p.x)) {
      // Compute the x-coordinate of the intersection of the segment with the ray
      double x = poly[i].x + (p.y - poly[i].y) * (poly[j].x - poly[i].x) / (poly[j].y - poly[i].y);
      if (p.x < x) {
        crossings++;
      }
    }
  }
  // If number of crossings is odd, the point is inside the polygon
  return (crossings % 2 == 1);
}



void move_guards(vector<point2d>& poly, vector<point2d>& guards, vector<point2d>& prev_movs){
  if (guards.size() == 0){ // if no guards have been placed there is nothing to do
    return;
  }
  assert(guards.size() != 0);
  for (int i = 0; i<guards.size(); i++){ // for each guard
    // if a near future version of itself is not in the polygon we know we want to change direction 
    point2d future_guard;
    future_guard.x = guards[i].x + 2*prev_movs[i].x;
    future_guard.y = guards[i].y + 2*prev_movs[i].y;
    if (!pointInPolygon(poly, future_guard)){
      // Generate a random angle between 0 and 2*pi radians
      double angle = ((double) rand() / (RAND_MAX)) * 2 * M_PI;

      //check that moving in this random direction puts the guard back in bounds
      point2d provisional_guard = future_guard;
      provisional_guard.x += cos(angle);
      provisional_guard.y += sin(angle);
      if (!pointInPolygon(poly, provisional_guard)){
        continue;
      }

      // Update the guard's position to the new x and y coordinates
      guards[i].x += cos(angle);
      guards[i].y += sin(angle);

      // Update previous move to random direction
      prev_movs[i].x = cos(angle);
      prev_movs[i].y = sin(angle);
      
      
    }
    else{
      //if the guard is in the polygon we can continue to move in the same direction as before
      guards[i].x += prev_movs[i].x;
      guards[i].y += prev_movs[i].y;
    }
  }
}


/* ****************************** */
/* This is the main render function. We registered this function to be
   called by GL to render the window. 
 */
void display(void) {

  glClear(GL_COLOR_BUFFER_BIT);
  //clear all modeling transformations 
  glMatrixMode(GL_MODELVIEW); 
  glLoadIdentity();


  /* The default GL window is [-1,1]x[-1,1] with the origin in the
     center.  The camera is at (0,0,0) looking down negative
     z-axis.  

     The points are in the range (0,0) to (WINSIZE,WINSIZE), so they
     need to be mapped to [-1,1]x [-1,1] */
  
  //First we scale down to [0,2] x [0,2] */ 
  glScalef(2.0/WINDOWSIZE, 2.0/WINDOWSIZE, 1.0);  
  /* Then we translate so the local origin goes in the middle of teh
     window to (-WINDOWSIZE/2, -WINDOWSIZE/2) */
  glTranslatef(-WINDOWSIZE/2, -WINDOWSIZE/2, 0); 
  
  //now we draw in our local coordinate system (0,0) to
  //(WINSIZE,WINSIZE), with the origin in the lower left corner.
  draw_polygon(poly); 

  draw_guard(guards);

  draw_visible_polygon(poly, guards);

  /* execute the drawing commands */
  glFlush();
}



/* ****************************** */
void keypress(unsigned char key, int x, int y) {
  switch(key) {
  case 'q':	
    exit(0);
    break;
  case 's': // to start drawing the polygon
    printf("drawing the polygon... remember to go anticlockwise!\n");
    poly.clear();
    guards.clear();
    poly_init_mode = 1;
    break;
  case 'e': // to stop drawing the polygon
    printf("you have stopped drawing the polygon\n");
    if (poly_init_mode && num_vertices > 2){
      poly_init_mode = 0;
      glutPostRedisplay();
      if(polygonSimple(poly)){
        printf("Is polygon simple: yes!\n");
      }
      else{
        printf("Is polygon simple: no! have another go mate. Press s to start...\n");
        poly.clear();
        glutPostRedisplay();
      }
    }
    break;
  case 'g': // to place guards
    printf("Place the guard(s) inside the art gallery.\n");
    guards.clear();
    poly_init_mode = 2;
    break;
  case 'a': // to get the guards to move randomly within the polygon
    animation_mode = !animation_mode;
    if (animation_mode == true){
      printf("animation halted!\n");
    }
    else{
      printf("animation started again!\n");
    }
    glutPostRedisplay();
    break;

  case 'm': // to toggle between a click placing a guard and moving the existing guards
    if (additional_guards == true){
      printf("Clicking will move the guards\n");
      additional_guards = false;
    }
    else{
      printf("clicking will add a new guard\n");
      additional_guards = true;
    }
  }
}


/* 
void glutMouseFunc(void (*func)(int button, int state, int x, int y));

glutMouseFunc sets the mouse callback for the current window. When a
user presses and releases mouse buttons in the window, each press and
each release generates a mouse callback. The button parameter is one
of GLUT_LEFT_BUTTON, GLUT_MIDDLE_BUTTON, or GLUT_RIGHT_BUTTON. For
systems with only two mouse buttons, it may not be possible to
generate GLUT_MIDDLE_BUTTON callback. For systems with a single mouse
button, it may be possible to generate only a GLUT_LEFT_BUTTON
callback. The state parameter is either GLUT_UP or GLUT_DOWN
indicating whether the callback was due to a release or press
respectively. The x and y callback parameters indicate the window
relative coordinates when the mouse button state changed. If a
GLUT_DOWN callback for a specific button is triggered, the program can
assume a GLUT_UP callback for the same button will be generated
(assuming the window still has a mouse callback registered) when the
mouse button is released even if the mouse has moved outside the
window.
*/
void mousepress(int button, int state, int x, int y) {

  if(state == GLUT_DOWN) { //mouse click detected 
    printf("poly_init_mode: %d\n",  poly_init_mode);

    //(x,y) are in window coordinates, where the origin is in the upper
    //left corner; our reference system has the origin in lower left
    //corner, this means we have to reflect y
    mouse_x = (double)x;
    mouse_y = (double)(WINDOWSIZE - y); 
    //printf("mouse pressed at (%.1f,%.1f)\n", mouse_x, mouse_y); 

    if (poly_init_mode == 1){ // s has been pressed and we want to add each mouse press to the polygon
      point2d p = {mouse_x, mouse_y};
      //add this point to poly
      poly.push_back(p);
      num_vertices++;
    }

    if (poly_init_mode == 2 && additional_guards == true){ // g has been pressed and we want to add a point to represent a guard
      point2d p = {mouse_x, mouse_y};
      point2d prev = {1,1};
      guards.push_back(p);
      prev_movs.push_back(prev);
      num_guards++;
      printf("Guard placed at: (%f, %f): \n", mouse_x, mouse_y);
    }
    else if (poly_init_mode == 2 && additional_guards == false){
      move_guards(poly, guards, prev_movs);
    }
  }
  
  glutPostRedisplay();
}





//this function is called every frame. Use for animations 
void timerfunc() {
  if (animation_mode == true){
    move_guards(poly, guards, prev_movs);
    glutPostRedisplay(); 
  }
}







