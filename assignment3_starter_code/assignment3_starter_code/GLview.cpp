#include "GLview.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <limits>
#include <math.h>

using namespace std;

float copysign0(float x, float y) { return (y == 0.0f) ? 0 : copysign(x,y); }

void Matrix2Quaternion(QQuaternion &Q, QMatrix4x4 &M) {
    Q.setScalar(sqrt( max( 0.0f, 1 + M(0,0)  + M(1,1) + M(2,2) ) ) / 2);
    Q.setX(sqrt( max( 0.0f, 1 + M(0,0) - M(1,1) - M(2,2) ) ) / 2);
    Q.setY(sqrt( max( 0.0f, 1 - M(0,0) + M(1,1) - M(2,2) ) ) / 2);
    Q.setZ(sqrt( max( 0.0f, 1 - M(0,0) - M(1,1) + M(2,2) ) ) / 2);
    Q.setX(copysign0( Q.x(), M(2,1) - M(1,2) ));  Q.setY(copysign0( Q.y(), M(0,2) - M(2,0) ));  Q.setZ(copysign0( Q.z(), M(1,0) - M(0,1) )) ;
}

// GLView constructor. DO NOT MODIFY.
GLview::GLview(QWidget *parent)  : QOpenGLWidget(parent)
{
    scaleFlag = translateFlag = rotateFlag = false;
    lastPosFlag = false;
    mesh = NULL;
}

GLview::~GLview()
{
    makeCurrent(); // When deleting the mesh, you need to grab and relase the opengl context.
    if(mesh != NULL) {
        delete mesh;
    }
    doneCurrent();
}

// Load object file. DO NOT MODIFY.
bool GLview::LoadOBJFile(const QString file)
{
    Mesh *newmesh = new Mesh;
    if(!newmesh->load_obj(file)) {
        makeCurrent(); // Make current openGL context.
        delete newmesh;
        doneCurrent(); // Make current openGL context.
        return false;
    }
    if(mesh != NULL) {
        makeCurrent(); // Make current openGL context.
        delete mesh;
        doneCurrent(); // release openGL context.
    }
    mesh = newmesh;
    update_mesh();
    return true;
}

// Set default GL parameters.
void GLview::initializeGL()
{
    initializeOpenGLFunctions();
    vao.create();
    if (vao.isCreated()) {
        vao.bind();
    }

    glClearColor(0.2, 0.2, 0.2, 1.0f );   // Set the clear color to black
    glEnable(GL_DEPTH_TEST);    // Enable depth buffer

    // Prepare a complete shader program...
    if ( !prepareShaderProgram(wire_shader,  ":/wireframe.vsh", ":/wireframe.fsh" ) ) return;

    // Initialize default camera parameters
    yfov = 55;
    neardist = 0.1; fardist = 100;
    eye = QVector3D(-3,3,3);
    lookCenter = QVector3D(0,0,0);
    lookUp = QVector3D(0,0,1);

    QMatrix4x4 view;
    view.lookAt(eye, lookCenter, lookUp);
    Matrix2Quaternion(camrot, view);
}

// Set the viewport to window dimensions. DO NOT MODIFY.
void GLview::resizeGL( int w, int h )
{
    glViewport( 0, 0, w, qMax( h, 1 ) );
}

void GLview::initCameraGL()
{
    QMatrix4x4 model, view, projection;

    // model.rotate(QQuaternion::slerp(q0slerp, q1slerp, slerpt).normalized());

    view.rotate(camrot); view.translate(-eye);
    projection.perspective(yfov, (float)width() / (float)height(), neardist, fardist);

    QMatrix4x4 model_view = view * model;
    QMatrix4x4 MVP = projection * model_view;

    wire_shader.bind();
    wire_shader.setUniformValue("MVP", MVP);
}

void GLview::paintGL()
{
    initCameraGL(); // Update lighting and camara position.

    // Clear the buffer with the current clearing color
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    if(mesh != NULL) {  wire_shader.bind(); glDrawArrays( GL_TRIANGLES, 0, 3*mesh->faces.size() );  }
}

void GLview::keyPressGL(QKeyEvent* e)
{
    switch ( e->key() ) {
        case Qt::Key_Escape:
        QCoreApplication::instance()->quit();
        break;
        // Set rotate, scale, translate modes (for track pad mainly).
        case Qt::Key_R: toggleRotate();    break;
        case Qt::Key_S: toggleScale();     break;
        case Qt::Key_T: toggleTranslate();     break;
        default: QOpenGLWidget::keyPressEvent( e );  break;
    }
}


// DO NOT MODIFY
bool GLview::prepareShaderProgram(QOpenGLShaderProgram &prep_shader, const QString &vertex_file, const QString &fragment_file)
{
    // First we load and compile the vertex shader.
    bool result = prep_shader.addShaderFromSourceFile( QOpenGLShader::Vertex, vertex_file );
    if ( !result ) qWarning() << prep_shader.log();

    // ...now the fragment shader...
    result = prep_shader.addShaderFromSourceFile( QOpenGLShader::Fragment, fragment_file );
    if ( !result ) qWarning() << prep_shader.log();

    // ...and finally we link them to resolve any references.
    result = prep_shader.link();
    if ( !result ) {   qWarning() << "Could not link shader program:" << prep_shader.log(); exit(1);   }
    return result;
}


// DO NOT MODIFY
void GLview::mousePressEvent(QMouseEvent *event)
{
    float px = event->x(), py = event->y();
    if (event->button()==Qt::RightButton) {
        ShowContextMenu(event);
    }
    if (event->button()==Qt::LeftButton) {
        if(mesh == NULL) return;
        float x = 2.0 * (px + 0.5) / float(width())  - 1.0,  y = -(2.0 * (py + 0.5) / float(height()) - 1.0);
        lastPosX = x;
        lastPosY = y;
        lastPosFlag = true;
        event->accept();
    }
}


// show the right-click popup menu. DO NOT MODIFY
void GLview::ShowContextMenu(const QMouseEvent *event)
{
  // You made add additional menu options for the extra credit below,
  // please use the template below to add additional menu options.
    QMenu menu;

    QAction* option0 = new QAction("Example", this);
    connect(option0, SIGNAL(triggered()), this, SLOT(process_example()));
    QAction* option1 = new QAction("Inflate", this);
    connect(option1, SIGNAL(triggered()), this, SLOT(inflate()));
    QAction* option2 = new QAction("Random Noise", this);
    connect(option2, SIGNAL(triggered()), this, SLOT(randomNoise()));
    QAction* option3 = new QAction("Split faces", this);
    connect(option3, SIGNAL(triggered()), this, SLOT(splitFaces()));
    QAction* option4 = new QAction("Center vertices tangentially", this);
    connect(option4, SIGNAL(triggered()), this, SLOT(centerVerticesTangentially()));
    QAction* option5 = new QAction("Split long edges", this);
    connect(option5, SIGNAL(triggered()), this, SLOT(splitLongEdges()));
    QAction* option6 = new QAction("Collapse short edges", this);
    connect(option6, SIGNAL(triggered()), this, SLOT(collapseShortEdges()));
    QAction* option7 = new QAction("Flip edges", this);
    connect(option7, SIGNAL(triggered()), this, SLOT(flipEdges()));
    QAction* option8 = new QAction("Bilateral Smoothing", this);
    connect(option8, SIGNAL(triggered()), this, SLOT(bilateralSmoothing()));
    QAction* option9 = new QAction("Mesh Simplification", this);
    connect(option9, SIGNAL(triggered()), this, SLOT(meshSimplification()));
    QAction* option10 = new QAction("Crop", this);
    connect(option10, SIGNAL(triggered()), this, SLOT(crop()));
    QAction* option11 = new QAction("Smooth", this);
    connect(option11, SIGNAL(triggered()), this, SLOT(smooth()));
    QAction* option12 = new QAction("Sharpen", this);
    connect(option12, SIGNAL(triggered()), this, SLOT(sharpen()));
    QAction* option13 = new QAction("Truncate", this);
    connect(option13, SIGNAL(triggered()), this, SLOT(truncate()));
    QAction* option14 = new QAction("Loop Subdivision", this);
    connect(option14, SIGNAL(triggered()), this, SLOT(loopSubdivision()));

    menu.addAction(option0);
    menu.addAction(option1);
    menu.addAction(option2);
    menu.addAction(option3);
    menu.addAction(option4);
    menu.addAction(option5);
    menu.addAction(option6);
    menu.addAction(option7);
    menu.addAction(option8);
    menu.addAction(option9);
    menu.addAction(option10);
    menu.addAction(option11);
    menu.addAction(option12);
    menu.addAction(option13);
    menu.addAction(option14);
    menu.exec(mapToGlobal(event->pos()));
}



// DO NOT MODIFY
void GLview::mouseMoveEvent(QMouseEvent *event)
{
    if(mesh == NULL) return;

    float px = event->x(), py = event->y();
    float x = 2.0 * (px + 0.5) / float(width())  - 1.0;
    float y = -(2.0 * (py + 0.5) / float(height()) - 1.0);

    // Record a last position if none has been set.
    if(!lastPosFlag) { lastPosX = x; lastPosY = y; lastPosFlag = true; return; }
    float dx = x - lastPosX, dy = y - lastPosY;
    lastPosX = x; lastPosY = y; // Remember mouse position.

    if (rotateFlag || (event->buttons() & Qt::LeftButton)) { // Rotate scene around a center point.
        float theta_y = 2.0 * dy / M_PI * 180.0f;
        float theta_x = 2.0 * dx / M_PI * 180.0f;

        QQuaternion revQ = camrot.conjugate();
        QQuaternion newrot = QQuaternion::fromAxisAndAngle(lookUp, theta_x);
        revQ = newrot * revQ;

        QVector3D side = revQ.rotatedVector(QVector3D(1,0,0));
        QQuaternion newrot2 = QQuaternion::fromAxisAndAngle(side, theta_y);
        revQ = newrot2 * revQ;
        revQ.normalize();

        camrot = revQ.conjugate().normalized();

        eye = newrot.rotatedVector(eye - lookCenter) + lookCenter;
        eye = newrot2.rotatedVector(eye - lookCenter) + lookCenter;
    }

    if (scaleFlag || (event->buttons() & Qt::MidButton)) { // Scale the scene.
        float factor = dx + dy;
        factor = exp(2.0 * factor);
        factor = (factor - 1.0) / factor;
        QVector3D translation = (lookCenter - eye) * factor;
        eye += translation;
    }
    if (translateFlag || (event->buttons() & Qt::RightButton)) { // Translate the scene.
        QQuaternion revQ = camrot.conjugate().normalized();
        QVector3D side = revQ.rotatedVector(QVector3D(1,0,0));
        QVector3D upVector = revQ.rotatedVector(QVector3D(0,1,0));

        float length = lookCenter.distanceToPoint(eye) * tanf(yfov * M_PI / 180.0f);
        QVector3D translation = -((side * (length * dx)) + (upVector * (length * dy) ));
        eye += translation;
        lookCenter += translation;
    }
    event->accept();
    update(); // Update display.
}

// To implement your methods, you should add a function to mesh and call as below.
void GLview::process_example()
{
    if(mesh == NULL) return;
    mesh->process_example();
    update_mesh();
}


void GLview::update_mesh()
{
    if(mesh == NULL) return;
    makeCurrent();
    mesh->storeVBO();

    // Update VBOs associated with shaders.
    wire_shader.bind();
    mesh->vertexBuffer.bind();
    wire_shader.setAttributeBuffer( "VertexPosition", GL_FLOAT, 0, 3 );
    wire_shader.enableAttributeArray( "VertexPosition" );

    mesh->baryBuffer.bind();
    wire_shader.setAttributeBuffer( "Barycentric", GL_FLOAT, 0, 3 );
    wire_shader.enableAttributeArray( "Barycentric" );
    doneCurrent();

    update();
}


// DO NOT MODIFY
void GLview::toggleRotate()
{
    if(mesh == NULL) return;
    translateFlag = scaleFlag = false;
    rotateFlag = !rotateFlag;
    setMouseTracking(rotateFlag);
    lastPosFlag = false;
}

// DO NOT MODIFY
void GLview::toggleScale()
{
    if(mesh == NULL) return;
    translateFlag = rotateFlag = false;
    scaleFlag = !scaleFlag;
    setMouseTracking(scaleFlag);
    lastPosFlag = false;
}

// DO NOT MODIFY
void GLview::toggleTranslate()
{
    if(mesh == NULL) return;
    rotateFlag = scaleFlag = false;
    translateFlag = !translateFlag;
    setMouseTracking(translateFlag);
    lastPosFlag = false;
}


// Note. After updating/modifying the mesh, you'll need to call update_mesh() below.

void GLview::inflate()
{
    cout << "Inflated!\n";
    // popup dialog box to get user input
    bool ok;
    double factor = QInputDialog::getDouble(this, tr("QInputDialog::getDouble()"),tr("Factor:"), 0, -5, numeric_limits<double>::max(), 2, &ok);
    if (!ok) {
        // warning message to notify user of bad input
        QMessageBox::information(this, tr("Application Name"), tr("Input value not in acceptable range. Please try a different value.") );
        return;
    }
    cout << factor << "\n";
    cout << copysign(10.0, -1.0) << "\n";

    //Compute the average edge lengths and normals
    mesh->compute_vert_normals();
    mesh->compute_avg_edge_lengths();
    //Move every vertex along its normal direction
    for (int v = 0; v < mesh->vertices.size(); v++){
      //Calculate the displacement of each vertex
      //Take the factor, multiply by the average edge length at vertex and displace that much
      float displacement = mesh->avg_edge_lengths[v].avg_edge_length * factor;
      mesh->vertices[v] += (mesh->vertex_normals[v].n * displacement);
    }
    update_mesh();
}

void GLview::randomNoise()
{
    cout << "Random Noise Added!" << endl;

    // popup dialog box to get user input
    bool ok;
    double factor = QInputDialog::getDouble(this, tr("QInputDialog::getDouble()"),tr("Factor:"), 0, numeric_limits<double>::min(), numeric_limits<double>::max(), 2, &ok);
    if (!ok) {
        // warning message to notify user of bad input
        QMessageBox::information(this, tr("Application Name"), tr("Input value not in acceptable range. Please try a different value.") );
        return;
    }
    cout << factor << "\n";

    //Compute average edge lengths and normals
    mesh->compute_avg_edge_lengths();
    //Move every vertex in a random direction a random amount where the max displacement is between 0 and factor * avg_edge_length per vertex
    for (int v = 0; v < mesh->vertices.size(); v++){
      //Generate a random vector
      QVector3D rand_v(qrand(), qrand(), qrand());
      float displacement = fmod(qrand(), (factor * mesh->avg_edge_lengths[v].avg_edge_length));
      QVector3D m_vert = mesh->vertices[v];
      QVector3D difference_v = rand_v - m_vert;
      difference_v.normalize();
      mesh->vertices[v] += (difference_v*displacement);
    }
    update_mesh();
}

//Helper function to find if a vertex exists in the list already or just returns it given indexes
int GLview::find_verts(QVector3D q, int idx){
  for (int i = 0; i < mesh->vertices.size(); i++){
    QVector3D vertex = mesh->vertices[i];
    if (q.x() == vertex.x() && q.y() == vertex.y() && q.z() == vertex.z()){
      return i;
    }
  }
  return idx;
}

void GLview::splitFaces()
{
    cout << "Faces Split!" << endl;

    size_t num_faces = mesh->faces.size();
    for (size_t i = 0; i < num_faces; i++){
      //Iterate over the faces List
      //Get each vertex
      long v0_idx = mesh->faces[0].vert[0];
      long v1_idx = mesh->faces[0].vert[1];
      long v2_idx = mesh->faces[0].vert[2];
      QVector3D a = mesh->vertices[v0_idx];
      QVector3D b = mesh->vertices[v1_idx];
      QVector3D c = mesh->vertices[v2_idx];

      //Compute midpoints
      QVector3D d((float)((a.x() + c.x()) / 2), (float)((a.y() + c.y()) / 2), (float)((a.z() + c.z()) / 2));
      QVector3D e((float)((b.x() + c.x()) / 2), (float)((b.y() + c.y()) / 2), (float)((b.z() + c.z()) / 2));
      QVector3D f((float)((a.x() + b.x()) / 2), (float)((a.y() + b.y()) / 2), (float)((a.z() + b.z()) / 2));

      //Add new vertices to List
      mesh->vertices.push_back(d);
      mesh->vertices.push_back(e);
      mesh->vertices.push_back(f);

      int idx_d = (int)mesh->vertices.size() - 3;
      int idx_e = (int)mesh->vertices.size() - 2;
      int idx_f = (int)mesh->vertices.size() - 1;
      idx_d = find_verts(d, idx_d);
      idx_e = find_verts(e, idx_e);
      idx_f = find_verts(f, idx_f);

      //Delete current face
      mesh->faces.erase(mesh->faces.begin());

      //Add 4 new faces at the corresponding vertices
      //Face 1
      vector<int> curr_face;
      curr_face.push_back(v0_idx);
      curr_face.push_back(idx_f);
      curr_face.push_back(idx_d);
      mesh->add_face(curr_face);
      curr_face.erase(curr_face.begin(), curr_face.begin() + 3);
      //Face 2
      curr_face.push_back(idx_f);
      curr_face.push_back(idx_e);
      curr_face.push_back(idx_d);
      mesh->add_face(curr_face);
      curr_face.erase(curr_face.begin(), curr_face.begin() + 3);
      //Face 3
      curr_face.push_back(idx_e);
      curr_face.push_back(idx_f);
      curr_face.push_back(v1_idx);
      mesh->add_face(curr_face);
      curr_face.erase(curr_face.begin(), curr_face.begin() + 3);
      //Face 4
      curr_face.push_back(v2_idx);
      curr_face.push_back(idx_d);
      curr_face.push_back(idx_e);
      mesh->add_face(curr_face);
      curr_face.erase(curr_face.begin(), curr_face.begin() + 3);
    }
    update_mesh();
}

void GLview::starFaces()
{
    cout << "implement starFaces()\n";
}

void GLview::splitLongEdges()
{
    cout << "implement splitLongEdges()\n";
}

void GLview::edge_collapse(int v0_idx, int v1_idx){
  QVector3D midpoint((float)((mesh->vertices[v0_idx].x() + mesh->vertices[v1_idx].x()) / 2), (float)((mesh->vertices[v0_idx].y() + mesh->vertices[v1_idx].y()) / 2), (float)((mesh->vertices[v0_idx].z() + mesh->vertices[v1_idx].z()) / 2));
  //Find and delete two faces that share the edge
  vector<Mesh_Face>::iterator it;

  mesh->vertices.push_back(midpoint);
  int m_idx = (int)mesh->vertices.size() - 1;

  for (it = mesh->faces.begin(); it < mesh->faces.end(); it++){
    Mesh_Face face = *it;
    bool contains_v0 = (face.vert[0] == v0_idx || face.vert[1] == v0_idx || face.vert[2] == v0_idx);
    bool contains_v1 = (face.vert[0] == v1_idx || face.vert[1] == v1_idx || face.vert[2] == v1_idx);

    if (contains_v0 && contains_v1){
      mesh->faces.erase(it);
      it--;
    } else {
      if (contains_v0){
        for (int i = 0; i < 3; i++){
          if (face.vert[i] == v0_idx) face.vert[i] = m_idx;
        }
      } else if (contains_v1){
        for (int i = 0; i < 3; i++){
          if (face.vert[i] == v1_idx) face.vert[i] = m_idx;
        }
      }
    }
  }
}

float GLview::average_total_edges(){
  float avg = 0;
  float edges = 0;
  for (int f = 0; f < mesh->faces.size(); f++){
    int v0_idx = mesh->faces[f].vert[0];
    int v1_idx = mesh->faces[f].vert[1];
    int v2_idx = mesh->faces[f].vert[2];
    QVector3D v0 = mesh->vertices[v0_idx];
    QVector3D v1 = mesh->vertices[v1_idx];
    QVector3D v2 = mesh->vertices[v2_idx];

    float dist_sum = v0.distanceToPoint(v1) + v0.distanceToPoint(v2) + v1.distanceToPoint(v2);
    avg += dist_sum;
    edges += 3;
  }

  avg = (avg / edges);
  return avg;
}

void GLview::collapseShortEdges()
{
    cout << "Collapsed Short Edges!" << endl;

    //Find 4/5th average edge length
    float collapse_threshold = average_total_edges() * 4/5;
    //LOL it's named f_it -> f it amirite?? haha
    vector<Mesh_Face>::iterator f_it;

    for (f_it = mesh->faces.begin(); f_it < mesh->faces.end(); f_it++){
      Mesh_Face f = *f_it;
      QVector3D v0 = mesh->vertices[f.vert[0]];
      QVector3D v1 = mesh->vertices[f.vert[1]];
      QVector3D v2 = mesh->vertices[f.vert[2]];

      //Calculate edge lendths
      float len_v0v1 = v0.distanceToPoint(v1);
      float len_v0v2 = v0.distanceToPoint(v2);
      float len_v1v2 = v1.distanceToPoint(v2);

      if (len_v0v1 < collapse_threshold){
        edge_collapse(f.vert[0], f.vert[1]);
      }

      if (len_v0v2 < collapse_threshold){
        edge_collapse(f.vert[0], f.vert[2]);
      }

      if (len_v1v2 < collapse_threshold){
        edge_collapse(f.vert[1], f.vert[2]);
      }
    }
}

void GLview::crop()
{
    cout << "implement crop()\n";
}

void GLview::centerVerticesTangentially()
{
    cout << "Center Vertices Tangentially()!" << endl;

    mesh->compute_vertex_neighbors();
    vector<QVector3D> n_vertices;

    for (int i = 0; i < mesh->vertices.size(); i++){
      vector<int> v_neighbors = mesh->adjacent_verts[i].neighbors;
      if (v_neighbors.size() > 0){
        QVector3D n_position(0, 0, 0);
        for (int j = 0; j < v_neighbors.size(); j++){
          n_position += mesh->vertices[v_neighbors[j]] / v_neighbors.size();
        }
        n_vertices.push_back(n_position);
      } else {
        n_vertices.push_back(mesh->vertices[i]);
      }
    }

    mesh->vertices = n_vertices;
    update_mesh();
}

void GLview::sharpen()
{
    cout << "implement sharpen()\n";
}

void GLview::truncate()
{
    cout << "implement truncate()\n";
}

void GLview::bilateralSmoothing()
{
    cout << "implement bilateralSmoothing()\n";
}

void GLview::meshSimplification()
{
    cout << "implement meshSimplification()\n";
}

void GLview::loopSubdivision()
{
    cout << "implement loopSubdivision()\n";
}

void GLview::flipEdges()
{
  cout << "implement flipEdges()\n";
}

void GLview::smooth() {
  cout << "Smoothed!" << endl;

  mesh->compute_avg_edge_lengths();
  mesh->compute_vertex_neighbors();

  vector<QVector3D> n_vertices;

  for (int i = 0; i < mesh->vertices.size(); i++){
    vector<int> v_neighbors = mesh->adjacent_verts[i].neighbors;
    vector<float> weights;
    double distance, weight, total_w = 0, sigma = mesh->avg_edge_lengths[i].avg_edge_length;

    for (int j = 0; j < v_neighbors.size() + 1; j++){
      if (j == v_neighbors.size()){
        //Handle current vertex
        weight = 1./(sigma*sqrt(2*3.14));
      } else {
        distance = mesh->vertices[i].distanceToPoint(mesh->vertices[v_neighbors[j]]);
        weight = (1./(sigma*sqrt(2*3.14)))*exp(-(distance*distance)/(2.*sigma*sigma));
      }

      weights.push_back(weight);
      total_w += weight;
    }

    QVector3D n_position(0,0,0);
    for (int j = 0; j < v_neighbors.size(); j++){
      n_position += weights[j] / total_w * mesh->vertices[v_neighbors[j]];
    }
    n_position += weights[weights.size()-1] / total_w * mesh->vertices[i];
    n_vertices.push_back(n_position);
  }

  mesh->vertices = n_vertices;
  update_mesh();
}
