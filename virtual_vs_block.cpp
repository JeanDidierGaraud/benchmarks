#include <Eigen/Core>
#include <Eigen/Dense>
#include <iostream>
#include <random>
#include <memory>

#include <timer.hpp>

#ifdef SLEEP
#include <unistd.h>
#else
#define sleep(A) {}
#endif

/** Idee:
    comparer le cout d'un calcul qui attaque en vecto les tableaux de donnees
    vs. une fonction virtual sur des petits objets
**/

#include "mesh.hpp"

int main(int argc, char** argv)
{
  //  timers :   10000000 10000000 -> 3s for the slowest method
  //  timers :   100000000 100000000 -> 30s

  int nb_vertices = 10, nb_cells = 8;
  int sleep_time = 0;
  if (argc>1) nb_vertices = std::atoi(argv[1]);
  if (argc>2) nb_cells = std::atoi(argv[2]);
  if (argc>3) sleep_time = std::atoi(argv[3]);
  std::cout << "#vertices=" << nb_vertices << std::endl;

  Mesh mesh(nb_vertices, nb_cells);
#ifndef NDEBUG
  std::cout << mesh.coordinates << std::endl;
#endif
  std::cout << "#cells=" << nb_cells << std::endl;
#ifndef NDEBUG
  std::cout << mesh.tetras << std::endl;
#endif

  if (sleep_time) std::cout << "#sleep_time=" << sleep_time << std::endl;

  sleep(sleep_time);
  { // Version 1: "boucles a la C"
    aset::asolve::Timer t0("C style", 0, false);
    double vol=0;
    double* coords = mesh.coordinates.data();
    int* tetras = mesh.tetras.data();

    for (int rk=0; rk<nb_cells; rk++) {
      int* cell_indices = tetras + 4*rk;  // cell_indices[0:4] are the 4 summits
      const int DIM=3;
      double* pt0 = coords+DIM*cell_indices[0]; // pointer to xyz of 1st summit
      double* pt1 = coords+DIM*cell_indices[1]; // pointer to xyz of 2nd summit
      double* pt2 = coords+DIM*cell_indices[2]; // pointer to xyz of 3rd summit
      double* pt3 = coords+DIM*cell_indices[3]; // pointer to xyz of 4th summit

      double e1[3]; for (int i=0; i<3; i++) e1[i] = pt1[i] - pt0[i];
      double e2[3]; for (int i=0; i<3; i++) e2[i] = pt2[i] - pt0[i];
      double e3[3]; for (int i=0; i<3; i++) e3[i] = pt3[i] - pt0[i];

      double e2xe3[3];
      e2xe3[0] = e2[1]*e3[2] - e2[2]*e3[1];
      e2xe3[1] = e2[2]*e3[0] - e2[0]*e3[2];
      e2xe3[2] = e2[0]*e3[1] - e2[1]*e3[0];
      vol += (e1[0]*e2xe3[0] + e1[1]*e2xe3[1] + e1[2]*e2xe3[2])/6.0;
    }
    t0.stop_and_display();
    std::cout << "volume(C style) = " << vol << std::endl;
  }

  sleep(sleep_time);
  { // Version 2: loop et calcul direct, a base de slice syntax
    aset::asolve::Timer t0("eigen view+slice", 0, false);
    using Eigen::placeholders::all;
    double vol=0;
    // iterate on rows: https://eigen.tuxfamily.org/dox/group__TutorialSTL.html
    for (const auto& tetra: mesh.tetras.rowwise()) {
      const auto& cell_coords = mesh.coordinates(tetra, all);   // slicing syntax
#ifndef NDEBUG
      std::cout << "tetra: " << tetra << "\n" << cell_coords << std::endl;
#endif
      Eigen::Vector3d e1 = (cell_coords.row(1) - cell_coords.row(0));
      Eigen::Vector3d e2 = (cell_coords.row(2) - cell_coords.row(0));
      Eigen::Vector3d e3 = (cell_coords.row(3) - cell_coords.row(0));
      vol += (e1.dot(e2.cross(e3)))/6.0;
    }
    t0.stop_and_display();
    std::cout << "volume(view+slice)1 = " << vol << std::endl;
  }


  sleep(sleep_time);
  { // Version 3: visitor object and virtual function
    aset::asolve::Timer t0("visitor+virtual",0,false);
    double vol=0;
    for (int rk=0; rk<nb_cells; rk++) {
      Tetra t(&mesh, rk);
      VirtualCell* cell = &t;
      vol += cell->volume();
    }
    t0.stop_and_display();
    std::cout << "volume(visitor+virtual) = " << vol << std::endl;
  }

  sleep(sleep_time);
  { // Version 4: visitor non virtual (simulates a function selected above the Block)
    aset::asolve::Timer t0("visitor non virtual",0,false);
    double vol=0;
    for (int rk=0; rk<nb_cells; rk++) {
      TetraDirect t(&mesh, rk);
      vol += t.volume();
    }
    t0.stop_and_display();
    std::cout << "volume(visitor non virtual) = " << vol << std::endl;
  }

  sleep(sleep_time);
  { // Version 5: visitor non virtual and avoid cell_coords copy(simulates a function selected above the Block)
    aset::asolve::Timer t0("visitor non virtual, no coord copy",0,false);
    double vol=0;
    for (int rk=0; rk<nb_cells; rk++) {
      TetraDirect t(&mesh, rk);
      vol += t.volume_direct();
    }
    t0.stop_and_display();
    std::cout << "volume(visitor non virtual, no coord copy) = " << vol << std::endl;
  }

  sleep(sleep_time);
  { // Version 6: same as previous, take TetraDirect out of the loop
    aset::asolve::Timer t0("visitor non virtual, no coord copy, set_rank",0,false);
    // Johann appelle cet objet une loupe, ou un decodeur, qui se ballade sur les donnees
    double vol=0;
    TetraDirect t(&mesh, 0);
    for (int rk=0; rk<nb_cells; rk++) {
      t.rank_ = rk;
      vol += t.volume_direct();
    }
    t0.stop_and_display();
    std::cout << "volume(visitor non virtual, no coord copy, set_rank) = " << vol << std::endl;
  }




}
