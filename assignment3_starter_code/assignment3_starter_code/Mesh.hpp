#ifndef __MESH_HPP__

#include <QtGui>
#include <QtOpenGL>

#include <iostream>
#include <map>
using namespace std;

struct Mesh_Face {
    Mesh_Face() {
        vert[0] = vert[1] = vert[2] = -1;
    }

    Mesh_Face(long v0, long v1, long v2) {
        vert[0] = v0; vert[1] = v1; vert[2] = v2;
    }
    long vert[3]; // indices (in the vertex array) of all vertices (mesh_vertex)
};

//Neighbors
struct Adjacents {
  Adjacents(){
    vertex_idx = -1;
  }

  Adjacents(int idx){
    vertex_idx = idx;
  }

  void add_adj(int idx){
    for (int i = 0; i < neighbors.size(); i++){
      if (neighbors[i] == idx){
        return;
      }
    }
    neighbors.push_back(idx);
  }
  int vertex_idx;
  vector<int> neighbors;
};

struct Edge_Length {
  Edge_Length() {
    avg_edge_length = 0;
    vertex_idx = -1;
    edges = 0;
  }

  Edge_Length(float avg_edge_len, int vert_idx, int div_by) {
    avg_edge_length = avg_edge_len;
    vertex_idx = vert_idx;
    edges = div_by;
  }
  float avg_edge_length;
  int vertex_idx;
  int edges;
};

struct Vertex_Normal {
  Vertex_Normal() {
    n = QVector3D(0, 0, 0);
    vertex_idx = -1;
    faces = 0;
  }

  Vertex_Normal(QVector3D normal, int vert_idx, int n_faces) {
    n = normal;
    vertex_idx = vert_idx;
    faces = n_faces;
  }
  QVector3D n;
  int vertex_idx;
  int faces;
};

struct Mesh {
    vector<QVector3D> vertices; // List of shared verticies.
    vector<Mesh_Face> faces; // Mesh faces.
    QOpenGLBuffer vertexBuffer, baryBuffer;

    //Created data structures
    vector<Edge_Length> avg_edge_lengths;
    vector<Vertex_Normal> vertex_normals;
    vector<Adjacents> adjacent_verts;

    bool load_obj(QString filename);
    void storeVBO();
    void recenter();
    void add_face(const vector<int> &cur_vert);
    void process_example();
    void compute_avg_edge_lengths();
    void compute_vert_normals();
    float calculate_tri_area(QVector3D v0, QVector3D v1, QVector3D v2);
    void compute_vertex_neighbors();
};

#endif // __MESH_HPP__
