//
// admm.h
// 
// Created by Vincent Q. Vu on 2014-07-08
// Copyright 2014 Vincent Q. Vu. All rights reserved
//

#ifndef __ADMM_H
#define __ADMM_H

#include <RcppArmadillo.h>
#include "walltime.h"

/**
 * @brief               Projection and selection ADMM algorithm
 * @details 
 * Implements an ADMM algorithm for solving the optimization problem:
 * \f[
 *   \max_{x \in C} \langle input, x \rangle - R(x)
 * \f]
 * This can be interpreted as a regularized support function where the 
 * regularizer is the function R(x). The working memory for this algorithm 
 * is passed in by reference to the function.
 * 
 * 
 * @param projection    A functor operator()(arma::mat&) that implements 
 *                      Euclidean projection onto a convex set
 * @param selection     A functor operator()(arma::mat&, const double&) that 
 *                      implements the proximal operator of scaled regularizer 
 * @param input         Input matrix
 * @param z             Solution matrix. Reference to an arma::mat of the same  
 *                      dimension as input
 * @param u             Dual variable matrix. Reference to an arma::mat of the same 
 *                      dimension as input
 * @param admm_penalty  Reference to the ADMM penalty parameter; it may change
 * @param admm_adjust   Factor by which the ADMM penalty can increase/decrease
 * @param maxiter       Maximum number of iterations
 * @param tolerance     Convergence tolerance level for the primal and dual 
 *                      residual norms
 * @return The number of iterations
 */
template <typename F, typename G> 
int admm(F projection, G selection, 
         const arma::mat& input, arma::mat& z, arma::mat& u, 
         double& admm_penalty, const double& admm_adjust,
         int maxiter, const double& tolerance)
{
  int niter;
  double rr, ss;
  arma::mat x(z.n_rows, z.n_cols), z_old(z.n_rows, z.n_cols);

  for(niter = 0; niter < maxiter; niter++) {
    // Store previous value of z
    z_old = z;

    // Projection
    x = z - u + (input / admm_penalty);
    projection(x);

    // Selection
    z = x + u;
    selection(z, 1.0 / admm_penalty);

    // Dual variable update
    u = u + x - z;

    // Compute primal and dual residual norms
    rr = arma::norm(x - z, "fro");
    ss = admm_penalty * arma::norm(z - z_old, "fro");

    // Check convergence criterion
    if(rr < tolerance && ss < tolerance) {
      niter++;
      break;
    }

    // Penalty adjustment (Boyd, et al. 2010)
    if(rr > 10.0 * ss) {
        admm_penalty = admm_penalty * admm_adjust;
        u = u / admm_adjust;
    } else if(ss > 10.0 * rr) {
        admm_penalty = admm_penalty / admm_adjust;
        u = u * admm_adjust;
    }
  }

  return niter;
}



// The same ADMM algorithm, but with timing and error information in each iteration
template <typename F, typename G> 
int admm_benchmark(
    F projection, G selection, 
    double ndim, const arma::mat& input, const arma::mat& truth,
    arma::mat& z, arma::mat& u, 
    double& admm_penalty, const double& admm_adjust,
    int maxiter, const double& tolerance,
    std::vector<double>& errs, std::vector<double>& times, int verbose
)
{
  int niter;
  double rr, ss;
  double t1, t2;
  arma::mat x(z.n_rows, z.n_cols), z_old(z.n_rows, z.n_cols);

  for(niter = 0; niter < maxiter; niter++) {
    if(verbose > 0)
      Rcpp::Rcout << "iter = " << niter << std::endl;

    t1 = get_wall_time();

    // Store previous value of z
    z_old = z;

    // Projection
    x = z - u + (input / admm_penalty);
    projection(x);

    // Selection
    z = x + u;
    selection(z, 1.0 / admm_penalty);

    // Dual variable update
    u = u + x - z;

    // Compute primal and dual residual norms
    rr = arma::norm(x - z, "fro");
    ss = admm_penalty * arma::norm(z - z_old, "fro");

    // Check convergence criterion
    if(rr < tolerance && ss < tolerance) {
      niter++;
      t2 = get_wall_time();
      times.push_back(t2 - t1);

      arma::sp_mat zsp(z);
      arma::vec eigval;
      arma::mat eigvec;
      arma::eigs_sym(eigval, eigvec, zsp, int(ndim));
      arma::mat proj = eigvec * eigvec.t();

      // errs.push_back(arma::norm(z - truth, "fro"));
      errs.push_back(arma::norm(proj - truth, "fro"));

      break;
    }

    // Penalty adjustment (Boyd, et al. 2010)
    if(rr > 10.0 * ss) {
      admm_penalty = admm_penalty * admm_adjust;
      u = u / admm_adjust;
    } else if(ss > 10.0 * rr) {
      admm_penalty = admm_penalty / admm_adjust;
      u = u * admm_adjust;
    }

    t2 = get_wall_time();
    times.push_back(t2 - t1);

    arma::sp_mat zsp(z);
    arma::vec eigval;
    arma::mat eigvec;
    arma::eigs_sym(eigval, eigvec, zsp, int(ndim));
    arma::mat proj = eigvec * eigvec.t();

    // errs.push_back(arma::norm(z - truth, "fro"));
    errs.push_back(arma::norm(proj - truth, "fro"));
  }

  return niter;
}

#endif
