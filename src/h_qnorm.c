// This code is obtained @ following link: 
// http://stackoverflow.com/questions/1662943/standard-normal-distribution-z-value-function-in-c-sharp
// I translate it from C# to C

// <summary>
// Quantile function (Inverse CDF) for the normal distribution.
// </summary>
// <param name="p">Probability.</param>
// <param name="mu">Mean of normal distribution.</param>
// <param name="sigma">Standard deviation of normal distribution.</param>
// <param name="lower_tail">If true, probability is P[X <= x], otherwise P[X > x].</param>
// <param name="log_p">If true, probabilities are given as log(p).</param>
// <returns>P[X <= x] where x ~ N(mu,sigma^2)</returns>
// <remarks>See https://svn.r-project.org/R/trunk/src/nmath/qnorm.c</remarks>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define true 1
#define false 0

char R_Q_P01_boundaries(double p, double _LEFT_, double _RIGHT_, char lower_tail, char log_p, double *ans)
{
  if (log_p)
  {
    if (p > 0.0)
    {
      *ans = NAN;
      return (true);
    }
    if (p == 0.0)
    {
      if (lower_tail == true)
        *ans = _RIGHT_;
      else
        *ans = _LEFT_;
      return (true);
    }
    if (p == -INFINITY)
    {
      if (lower_tail == true)
        *ans = _LEFT_;
      else
        *ans = _RIGHT_;
      return (true);
    }
  }
  else
  {
    if (p < 0.0 || p > 1.0)
    {
      *ans = NAN;
      return (true);
    }
    if (p == 0.0)
    {
      if (lower_tail == true)
        *ans = _LEFT_;
      else
        *ans = _RIGHT_;
      //*ans = lower_tail ? _LEFT_ : _RIGHT_;
      return (true);
    }
    if (p == 1.0)
    {
      if (lower_tail == true)
        *ans = _RIGHT_;
      else
        *ans = _LEFT_;
      //*ans = lower_tail ? _RIGHT_ : _LEFT_;
      return (true);
    }
  }
  *ans = NAN;
  return (false);
}

double expM1(double x) 
{
  if (abs(x) < 1e-5)
    return x + 0.5 * x * x;
  else
    return exp(x) - 1.0;
}

double R_D_Lval(double p, char lower_tail) 
{
  return lower_tail ? p : 0.5 - p + 0.5; 
} 

double R_D_Cval(double p, char lower_tail) 
{ 
  return lower_tail ? 0.5 - p + 0.5 : p;
}

double R_DT_qIv(double p, char lower_tail, char log_p)
{
  return (log_p ? (lower_tail ? exp(p) : -expM1(p)) : R_D_Lval(p, lower_tail));
}

double R_DT_CIv(double p, char lower_tail, char log_p)
{
  return (log_p ? (lower_tail ? -expM1(p) : exp(p)) : R_D_Cval(p, lower_tail));
}

double QNorm(double p, double mu, double sigma, char lower_tail, char log_p)
{
  if (isnan(p) || isnan(mu) || isnan(sigma)) return (p + mu + sigma);
  double ans;
  char isBoundaryCase = R_Q_P01_boundaries(p, INFINITY, -INFINITY, lower_tail, log_p, &ans);
  if ( true == isBoundaryCase) return (ans);
  if (sigma < 0) return (NAN);
  if (sigma == 0) return (mu);

  double p_ = R_DT_qIv(p, lower_tail, log_p);
  double q = p_ - 0.5;
  double r, val;

  if (abs(q) <= 0.425)  // 0.075 <= p <= 0.925
  {
    r = .180625 - q * q;
    val = q * (((((((r * 2509.0809287301226727 +
                    33430.575583588128105) * r + 67265.770927008700853) * r +
                45921.953931549871457) * r + 13731.693765509461125) * r +
            1971.5909503065514427) * r + 133.14166789178437745) * r +
        3.387132872796366608)
      / (((((((r * 5226.495278852854561 +
                      28729.085735721942674) * r + 39307.89580009271061) * r +
                  21213.794301586595867) * r + 5394.1960214247511077) * r +
              687.1870074920579083) * r + 42.313330701600911252) * r + 1.0);
  }
  else
  {
    r = q > 0 ? R_DT_CIv(p, lower_tail, log_p) : p_;
    r = sqrt(-((log_p && ((lower_tail && q <= 0) || (!lower_tail && q > 0))) ? p : log(r)));

    if (r <= 5)              // <==> min(p,1-p) >= exp(-25) ~= 1.3888e-11
    {
      r -= 1.6;
      val = (((((((r * 7.7454501427834140764e-4 +
                      .0227238449892691845833) * r + .24178072517745061177) *
                  r + 1.27045825245236838258) * r +
                3.64784832476320460504) * r + 5.7694972214606914055) *
            r + 4.6303378461565452959) * r +
          1.42343711074968357734)
        / (((((((r *
                        1.05075007164441684324e-9 + 5.475938084995344946e-4) *
                      r + .0151986665636164571966) * r +
                    .14810397642748007459) * r + .68976733498510000455) *
                r + 1.6763848301838038494) * r +
              2.05319162663775882187) * r + 1.0);
    }
    else                     // very close to  0 or 1 
    {
      r -= 5.0;
      val = (((((((r * 2.01033439929228813265e-7 +
                      2.71155556874348757815e-5) * r +
                    .0012426609473880784386) * r + .026532189526576123093) *
                r + .29656057182850489123) * r +
              1.7848265399172913358) * r + 5.4637849111641143699) *
          r + 6.6579046435011037772)
        / (((((((r *
                        2.04426310338993978564e-15 + 1.4215117583164458887e-7) *
                      r + 1.8463183175100546818e-5) * r +
                    7.868691311456132591e-4) * r + .0148753612908506148525)
                * r + .13692988092273580531) * r +
              .59983220655588793769) * r + 1.0);
    }
    if (q < 0.0) val = -val;
  }

  return (mu + sigma * val);
}

/*
   int main() {
   double p, mu, sigma;
   char lower_tail = false, log_p = false;
   double result;

   while(true) {
   scanf("%lf", &p);
   scanf("%lf", &mu);
   scanf("%lf", &sigma);
   result = QNorm(p, mu, sigma, lower_tail, log_p);
   printf("%lf, %lf, %lf, %lf", p, mu, sigma, result);
   }
   return result;
   }*/
