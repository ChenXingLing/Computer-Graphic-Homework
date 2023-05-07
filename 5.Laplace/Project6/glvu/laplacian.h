#pragma once
#include <vector>

#include <Eigen/Core>
#include <Eigen/Sparse>

//#define Tri Eigen::Triplet<double>
template<class MatF, class MatI> 
auto Laplacian(const MatF &X, const MatI &T)//X：点坐标，T：三角面点组
{
    std::vector<Eigen::Triplet<double>> ijv;

    // TODO 1: compute ijv triplet for the sparse Laplacian


    //////////////////////////////////////////////////////////////////
    int nv = X.rows();
    Eigen::SparseMatrix<double, Eigen::ColMajor> M(nv, nv);

    M.setFromTriplets(ijv.cbegin(), ijv.cend());

    return M;
}


