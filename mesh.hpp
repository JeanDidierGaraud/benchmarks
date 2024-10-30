#pragma once
/**
   A simple Mesh class, for benchmarks.
 **/

class Mesh {  // data class
public:

  Mesh(int nb_vertices, int nb_cells) {
    //https://eigen.tuxfamily.org/dox/classEigen_1_1PlainObjectBase.html#title80
    // Numbers are uniformly spread through
    //  their whole definition range for integer types,
    //  and in the [-1:1] range for floating point scalar types.
    coordinates.setRandom(nb_vertices, 3);

    // tetras.setRandom(nb_cells,4); --> pas bon, pas les bonnes bornes, il faut tricoter un peu plus:
#ifdef REALLY_RANDOM
    std::random_device rd;
    std::mt19937 gen(rd());
#else  // reproducible random
    std::default_random_engine gen;
#endif
    std::uniform_int_distribution<int> dis(0, nb_vertices-1);
    tetras = Eigen::MatrixXi::NullaryExpr(nb_cells, 4, [&](){return dis(gen);});
  }

  Eigen::Matrix<double, Eigen::Dynamic, 3> coordinates;
  Eigen::Array<int, Eigen::Dynamic, 4> tetras;
};

using CellCoordinates = Eigen::Matrix<double, Eigen::Dynamic, 3>;  // pourrait etre static dans Tetra ; pourrait aussi etre un temporaire (a voir ce que retourne array.row(i)

class VirtualCell {
  //  std::weak_ptr<Mesh> parent_mesh_;  // fixme voir comment ca fonctionne et si ca ajoute un surcout
  Mesh* parent_mesh_;
  int rank_;

public:
  VirtualCell(Mesh* mesh, int rank): parent_mesh_(mesh), rank_(rank) {}
  virtual ~VirtualCell() = default;
  virtual CellCoordinates coordinates() const {    // fixme: pas trouve comment return une const ref& : ca declenche un warning: returning reference to temporary
    const auto& tetra = parent_mesh_->tetras.row(rank_);
    return parent_mesh_->coordinates(tetra, Eigen::placeholders::all);
  }
  virtual double volume() {return 0;}
};

/**
   @brief A Tetrehadron, derived virtual class.
 **/
class Tetra: public VirtualCell {
public:
  Tetra(Mesh* mesh, int rank): VirtualCell(mesh, rank){}

  //  using CellCoordinates = Eigen::Matrix<double, 4, 3>;  // ai-je le droit de surcharger ce typdeef? si oui, on y gagne (surement un peu parce qu'on saura 4x3 a la compil...
  virtual double volume() {
    const auto& cell_coords = coordinates();
    Eigen::Vector3d e1 = (cell_coords.row(1) - cell_coords.row(0));
    Eigen::Vector3d e2 = (cell_coords.row(2) - cell_coords.row(0));
    Eigen::Vector3d e3 = (cell_coords.row(3) - cell_coords.row(0));
    return (e1.dot(e2.cross(e3)))/6.0;
  }

};



/**
   @brief A Tetrehadron, avoiding virtual class.
 **/
class TetraDirect {
  const Mesh* parent_mesh_;
public:
  int rank_;
  TetraDirect(const Mesh* mesh, int rank): parent_mesh_(mesh), rank_(rank) {}

  //  const auto& coordinates() const {  // chouette, ici je peux "return const auto&" auto deduction ... ah ben non warning+segfault!
  CellCoordinates coordinates() const {  // fixme: eviter cette copie
    const auto& tetra = parent_mesh_->tetras.row(rank_);
    return parent_mesh_->coordinates(tetra, Eigen::placeholders::all);
  }
  double volume() const {
    const auto& cell_coords = coordinates();
    Eigen::Vector3d e1 = (cell_coords.row(1) - cell_coords.row(0));
    Eigen::Vector3d e2 = (cell_coords.row(2) - cell_coords.row(0));
    Eigen::Vector3d e3 = (cell_coords.row(3) - cell_coords.row(0));
    return (e1.dot(e2.cross(e3)))/6.0;
  }

  double volume_direct() const {
    const auto& tetra = parent_mesh_->tetras.row(rank_);
    const auto& cell_coords = parent_mesh_->coordinates(tetra, Eigen::placeholders::all);
    Eigen::Vector3d e1 = (cell_coords.row(1) - cell_coords.row(0));
    Eigen::Vector3d e2 = (cell_coords.row(2) - cell_coords.row(0));
    Eigen::Vector3d e3 = (cell_coords.row(3) - cell_coords.row(0));
    return (e1.dot(e2.cross(e3)))/6.0;
  }

};
