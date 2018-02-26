#include <iostream>
#include <fstream>
#include <iomanip>

#include "Mesh.hpp"

using namespace std;

void Mesh::add_face(const vector<int> &cur_vert) {
    if(cur_vert.size() > 3) {
        // If number of edges in face is greater than 3,
        // decompose into triangles as a triangle fan.
        int v0 = cur_vert[0], v1 = cur_vert[1], v2 = cur_vert[2];

        faces.push_back(Mesh_Face(v0, v1, v2));     // First face

        // all subsequent faces
        for( size_t i = 3; i < cur_vert.size(); i++ ) {
            v1 = v2; v2 = cur_vert[i];
            faces.push_back(Mesh_Face(v0, v1, v2));
        }
    }
    else if(cur_vert.size() == 3) {
        faces.push_back(Mesh_Face(cur_vert[0], cur_vert[1], cur_vert[2]));
    }
}

bool Mesh::load_obj(QString filename) {
    QFile objfile(filename);
    if (!objfile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false; //error
    }
    QTextStream in(&objfile);
    long face_cnt = 0;

    while (!in.atEnd()) {
        QString line = in.readLine();
        line = line.trimmed();
        line = line.replace("\t", " ");

        QStringList tokens = line.trimmed().split(' ', QString::SkipEmptyParts);
        if(tokens.size() == 0) continue;

        if(tokens[0] == "v") {
            if(tokens.size() < 4) return false; // eror
            float x = tokens[1].toFloat();
            float y = tokens[2].toFloat();
            float z = tokens[3].toFloat();
            vertices.push_back(QVector3D(x,y,z));
        }
        if(tokens[0] == "f") {
            vector<int> cur_vert;
            for(int i = 1; i < tokens.size(); i++) {
                QStringList indexes = tokens[i].split("/");
                if(indexes.size() >= 1) {
                    if(indexes[0].toLong() < 0) {  cur_vert.push_back(vertices.size() + indexes[0].toLong()); }
                    else { cur_vert.push_back(indexes[0].toLong() - 1); }
                }
            }
            face_cnt++;
            add_face(cur_vert);
        }
    }
    cout << "face_cnt=" << face_cnt << endl;
    cout << "faces.size()=" << faces.size() << endl;
    cout << "vertices.size()=" << vertices.size() << endl;

    recenter();
    return true;
}

void Mesh::recenter() {
    if( vertices.size() < 1) return;
    QVector3D maxPoint = vertices[0];
    QVector3D minPoint = vertices[0];

    // Find the AABB
    for( uint i = 0; i < vertices.size(); ++i ) {
        QVector3D & point = vertices[i];
        if( point[0] > maxPoint[0] ) maxPoint[0] = point[0];
        if( point[1] > maxPoint[1] ) maxPoint[1] = point[1];
        if( point[2] > maxPoint[2] ) maxPoint[2] = point[2];
        if( point[0] < minPoint[0] ) minPoint[0] = point[0];
        if( point[1] < minPoint[1] ) minPoint[1] = point[1];
        if( point[2] < minPoint[2] ) minPoint[2] = point[2];
    }

    // Center of the AABB
    QVector3D center = QVector3D( (maxPoint[0] + minPoint[0]) / 2.0f,
    (maxPoint[1] + minPoint[1]) / 2.0f,
    (maxPoint[2] + minPoint[2]) / 2.0f );

    // Translate center of the AABB to the origin
    for( uint i = 0; i < vertices.size(); ++i ) {
        QVector3D & point = vertices[i];
        point = point - center;
    }
}


void Mesh::process_example() {
    for(size_t v = 0; v < vertices.size(); v++) {
        if(vertices[v][0] > 0) {
            vertices[v][0] += 3.5;
        }
    }
}

void Mesh::compute_vertex_neighbors(){
  cout << "Computed neighbors" << endl;
  //Clear out everything
  adjacent_verts.erase(adjacent_verts.begin(), adjacent_verts.end());
  for (int i = 0; i < vertices.size(); i++){
    //Init all vertex indexes
    adjacent_verts.push_back(Adjacents(i));
  }

  for (int f = 0; f < faces.size(); f++){
    //Get the vertices of the triangle
    int v0_idx = faces[f].vert[0];
    int v1_idx = faces[f].vert[1];
    int v2_idx = faces[f].vert[2];
    adjacent_verts[v0_idx].add_adj(v1_idx);
    adjacent_verts[v0_idx].add_adj(v2_idx);
    adjacent_verts[v1_idx].add_adj(v0_idx);
    adjacent_verts[v1_idx].add_adj(v2_idx);
    adjacent_verts[v2_idx].add_adj(v0_idx);
    adjacent_verts[v2_idx].add_adj(v1_idx);
  }

  /*for (int i = 0; i < adjacent_verts.size(); i++){
    cout << "Vertex idx: " << adjacent_verts[i].vertex_idx << " Number of Adjacents: " << (int)adjacent_verts[i].neighbors.size() << endl;
  }*/
}

void Mesh::compute_avg_edge_lengths(){
  cout << "Computed average edge length" << endl;

  //Iterate over vertices and set all the edge lengths to zero
  avg_edge_lengths.erase(avg_edge_lengths.begin(), avg_edge_lengths.end());
  for (int v = 0; v < vertices.size(); v++){
    //Set default average edge lengths to zero
    //OVO Sound ;)
    avg_edge_lengths.push_back(Edge_Length(0, v, 0));
  }

  for (int f = 0; f < faces.size(); f++){
    //Get the vertices of the triangle
    QVector3D v0 = vertices[faces[f].vert[0]];
    QVector3D v1 = vertices[faces[f].vert[1]];
    QVector3D v2 = vertices[faces[f].vert[2]];
    //Calculate the distance between points
    float distancev0v1 = v0.distanceToPoint(v1);
    float distancev0v2 = v0.distanceToPoint(v2);
    float distancev1v2 = v1.distanceToPoint(v2);
    //Add it to the current total and the number of edges
    avg_edge_lengths[faces[f].vert[0]].avg_edge_length += distancev0v2;
    avg_edge_lengths[faces[f].vert[0]].avg_edge_length += distancev0v1;
    avg_edge_lengths[faces[f].vert[0]].edges += 2;
    avg_edge_lengths[faces[f].vert[1]].avg_edge_length += distancev0v1;
    avg_edge_lengths[faces[f].vert[1]].avg_edge_length += distancev1v2;
    avg_edge_lengths[faces[f].vert[1]].edges += 2;
    avg_edge_lengths[faces[f].vert[2]].avg_edge_length += distancev0v2;
    avg_edge_lengths[faces[f].vert[2]].avg_edge_length += distancev1v2;
    avg_edge_lengths[faces[f].vert[2]].edges += 2;
  }

  for (int v = 0; v < vertices.size(); v++){
    //Compute and store average
    avg_edge_lengths[v].avg_edge_length = avg_edge_lengths[v].avg_edge_length / avg_edge_lengths[v].edges;
  }
}

void Mesh::compute_vert_normals(){
  cout << "Computed per vertex normals" << endl;

  //Set all normals initially to zero
  vertex_normals.erase(vertex_normals.begin(), vertex_normals.end());
  for (int v = 0; v < vertices.size(); v++){
    vertex_normals.push_back(Vertex_Normal(QVector3D(0, 0, 0), v, 0));
  }

  for (int f = 0; f < faces.size(); f++){
    //Get the vertices of the triangle
    QVector3D v0 = vertices[faces[f].vert[0]];
    QVector3D v1 = vertices[faces[f].vert[1]];
    QVector3D v2 = vertices[faces[f].vert[2]];
    //Calculate the surface normal
    QVector3D normal_vec = QVector3D::crossProduct(v1-v0, v2-v1);
    normal_vec.normalize();
    //Calcuate the area of the triangle face for weight
    float tri_area = calculate_tri_area(v0, v1, v2);

    //Add normal multiplied by weight area to the running total of the normal
    vertex_normals[faces[f].vert[0]].n += normal_vec * tri_area;
    vertex_normals[faces[f].vert[0]].faces++;

    vertex_normals[faces[f].vert[1]].n += normal_vec * tri_area;
    vertex_normals[faces[f].vert[1]].faces++;

    vertex_normals[faces[f].vert[2]].n += normal_vec * tri_area;
    vertex_normals[faces[f].vert[2]].faces++;
  }

  //Average the final normals and normalize them to unit length
  for (int vn = 0; vn < vertex_normals.size(); vn++){
    vertex_normals[vn].n /= vertex_normals[vn].faces;
    vertex_normals[vn].n.normalize();
  }
}

float Mesh::calculate_tri_area(QVector3D v0, QVector3D v1, QVector3D v2){
  //Calculate
  QVector3D cross_prod = QVector3D::crossProduct((v1-v0),(v2-v1));
  float mag = sqrt((cross_prod.x()*cross_prod.x() + cross_prod.y()*cross_prod.y() + cross_prod.z()*cross_prod.z()));
  return (mag / 2);
}

void Mesh::storeVBO() {
    vector<QVector3D> tri_vert, tri_bary;

    for(long f = 0; f < (long)faces.size(); f++) {
        tri_vert.push_back(vertices.at(faces[f].vert[0]));
        tri_vert.push_back(vertices.at(faces[f].vert[1]));
        tri_vert.push_back(vertices.at(faces[f].vert[2]));

        tri_bary.push_back(QVector3D(1,0,0));;
        tri_bary.push_back(QVector3D(0,1,0));;
        tri_bary.push_back(QVector3D(0,0,1));;
    }

    if(vertexBuffer.isCreated()) vertexBuffer.destroy();
    vertexBuffer.create();
    vertexBuffer.setUsagePattern( QOpenGLBuffer::StaticDraw );
    vertexBuffer.bind();
    vertexBuffer.allocate(&tri_vert[0] , sizeof( QVector3D ) * tri_vert.size());

    if(baryBuffer.isCreated()) baryBuffer.destroy();
    baryBuffer.create();
    baryBuffer.setUsagePattern( QOpenGLBuffer::StaticDraw );
    baryBuffer.bind();
    baryBuffer.allocate(&tri_bary[0] , sizeof( QVector3D ) * tri_bary.size());
}
