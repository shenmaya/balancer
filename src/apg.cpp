
#include <RcppArmadillo.h>
//#include <Rcpp.h>
#include <numeric>
#include "balancer_types.h"

using namespace arma;
using namespace Rcpp;


//' Accelerated proximal gradient method
//'
//' @param grad_ptr Pointer to gradient function
//' @param prox_ptr Pointer to prox function
//' @param loss_opts List of options for loss (input data, tuning params, etc.)
//' @param prox_opts List of options for prox (regularization parameter)
//' @param x Initial value
//' @param max_it Maximum number of iterations
//' @param eps Convergence tolerance
//' @param beta Backtracking line search parameter
//'
//' @return Optimal value
//' @export
// [[Rcpp::export]]
mat apg(gptr grad_ptr,
        pptr prox_ptr,
        List loss_opts,
        List prox_opts,
        mat x,
        int max_it,
        double eps, double alpha,
        double beta, bool accel) {

  int dim1 = x.n_rows;
  int dim2 = x.n_cols;
  // grab the functions from pointers
  gradPtr grad_f = *grad_ptr;
  proxPtr prox_h = *prox_ptr;
  

  mat y = x;
  // accelerated
  double theta = 1;
  mat oldx;
  mat oldy;
  mat gtx;
  mat grad;
  mat oldg;
  double fx;
  double fgtx;
  double improve;
  double diff;
  double t = 1.0;
  double oldt;
  double t_hat;
  bool backcond;
  int j;

  // step size initialization
  grad = grad_f(y, loss_opts);
  t = 1 / sqrt(accu(pow(grad,2)));
  mat x_hat = x - t * grad;
  mat g_hat = grad_f(x_hat, loss_opts);
  double num = accu((x - x_hat) % (grad - g_hat));
  double denom = accu(pow(grad - g_hat,2));
  t = fabs(num / denom);

  for(int i = 1; i <= max_it; i++) {
    // Rcout << i << "\n";
    // Rcout << t << "\n";
    oldx = mat(x);
    oldy = mat(y);
    
    x = prox_h(y - t * grad, t, prox_opts);

    // Rcout << accu(pow(y - x,2)) << "\n";
    // stopping criterion
    if(accu(pow(y - x,2)) < eps) {
      break;
    }

    if(accel) {
      theta = 2 / (1 + sqrt(1 + 4/(pow(theta,2))));
    } else {
      theta = 1;
    }

    // restart
    if(dot(grad, (x - oldx)) > 0) {
      x = oldx;
      y = x;
      theta = 1;
    }
    

    y = x + (1-theta) * (x - oldx);

    oldg = mat(grad);
    grad = grad_f(y, loss_opts);
    
    t_hat = 0.5 * accu(pow(y - oldy, 2)) /
      fabs(accu((y - oldy) % (oldg - grad)));

    double maxval = (t_hat > beta * t) ? t_hat : beta * t;
    t = (alpha * t < maxval) ? alpha * t : maxval;

    if((i % 100) == 0) {
      Rcout << i << "\n";
      Rcout << t << "\n";
      Rcpp::checkUserInterrupt();
    }
  }

  return(x);
}
