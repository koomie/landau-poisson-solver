/* This is the source file which contains the subroutines necessary for calculating the electric potential and the
 * corresponding field, as well as any integrals which involve the field for the sake of the collisionless advection
 * problem.
 *
 * Functions included: Gridv, Gridx, rho_x, rho, computePhi_x_0, computePhi, PrintPhiVals, computeC_rho, Int_Int_rho
 * Int_Int_rho1st, Int_E, Int_E1st, Int_E2nd, Int_fE, I1, I2, I3, I5, computeH, RK3
 *
 */

#include "FieldCalculations.h"																					// FieldCalculations.h is where the prototypes for the functions contained in this file are declared

double rho_x(double x, double *U, int i) // for x in I_i
{
  int j, k;
  double tmp=0.;
  //#pragma omp parallel for shared(U) reduction(+:tmp)
  for(j=0;j<size_v;j++){
	k=i*size_v + j;
	tmp += U[k*6+0] + U[k*6+1]*(x-Gridx((double)i))/dx + U[k*6+5]/4.;
  }
  
  return dv*dv*dv*tmp;
}

double rho(double *U, int i) //  \int f(x,v) dxdv in I_i * K_j
{
  int j, k;
  double tmp=0.;
 // #pragma omp parallel for shared(U) reduction(+:tmp)
  for(j=0;j<size_v;j++){
	k=i*size_v + j;
	tmp += U[k*6+0] + U[k*6+5]/4.;
  }
  
  return dx*dv*dv*dv*tmp;
}

double computePhi_x_0(double *U) // compute the constant coefficient of x in phi, which is actually phi_x(0) (Calculate C_E in the paper -between eq. 52 & 53?)
{
	int i, j, k, m, q;
	double tmp=0.;

	//#pragma omp parallel for private(j,q,m,k) shared(U) reduction(+:tmp) //reduction may change the final result a little bit
	for(j=0;j<size_v;j++){
		//j = j1*Nv*Nv + j2*Nv + j3;
		for(q=0;q<Nx;q++){
			for(m=0;m<q;m++){
				k=m*size_v + j;
				tmp += U[k*6] + U[k*6+5]/4.;
			}
			k=q*size_v + j;
			tmp += 0.5*(U[k*6+0] + U[k*6+5]/4.) - U[k*6+1]/12.;
		}
	}
	tmp = tmp*scalev*dx*dx;

	return 0.5*Lx - tmp/Lx;
}

double computePhi(double *U, double x, int ix)											// function to compute the potential Phi at a position x, contained in [x_(ix-1/2), x_(ix+1/2)]
{
	int i_out, i, j1, j2, j3, iNNN, j1NN, j2N, k;										// declare counters i_out (for the outer sum of i values), i, j1, j2, j3 for summing the contribution from cell I_i x K_(j1, j2, j3), iNNN (the value of i*Nv^3), j1NN (the value of j1*Nv^2), j2N (the value of j2*N) & k (the location in U of the cell I_i x K_(j1, j2, j3))
	double retn, sum1, sum3, sum4, x_diff, x_diff_mid, x_diff_sq, x_eval, C_E;			// declare retn (the value of Phi returned at the end), sum1 (the value of the first two sums), sum3 (the value of the third sum), sum4 (the value of the fourth sum), x_diff (the value of x - x_(ix-1/2)), x_diff_mid (the value of x - x_ix), x_diff_sq (the value of x_diff^2), x_eval (the value associated to the integral of (x - x_i)^2) & C_E (the value of the constant in the formula for phi)
	sum1 = 0;
	sum3 = 0;
	sum4 = 0;
	retn = 0;
	x_diff = x - Gridx(ix-0.5);
	x_diff_mid = x - Gridx(ix);
	x_diff_sq = x_diff*x_diff;
	x_eval = x_diff_mid*x_diff_mid*x_diff_mid/(6.*dx) - dx*x_diff_mid/8. - dx*dx/24.;

	for(i_out = 0; i_out < ix; i_out++)
	{
		for(i = 0; i < i_out; i++)
		{
			iNNN = i*Nv*Nv*Nv;
			for(j1 = 0; j1 < Nv; j1++)
			{
				j1NN = j1*Nv*Nv;
				for(j2 = 0; j2 < Nv; j2++)
				{
					j2N = j2*Nv;
					for(j3 = 0; j3 < Nv; j3++)
					{
						k = iNNN + j1NN + j2N + j3;
						sum1 += U[6*k] + U[6*k+5]/4.;
					}
				}
			}
		}
		iNNN = i_out*Nv*Nv*Nv;
		for(j1 = 0; j1 < Nv; j1++)
		{
			j1NN = j1*Nv*Nv;
			for(j2 = 0; j2 < Nv; j2++)
			{
				j2N = j2*Nv;
				for(j3 = 0; j3 < Nv; j3++)
				{
					k = iNNN + j1NN + j2N + j3;
					sum1 += U[6*k]/2 - U[6*k+1]/12. +  U[6*k+5]/8;
				}
			}
		}
	}
	sum1 = sum1*dx*dx;
	for(i = 0; i < i_out; i++)
	{
		iNNN = i*Nv*Nv*Nv;
		for(j1 = 0; j1 < Nv; j1++)
		{
			j1NN = j1*Nv*Nv;
			for(j2 = 0; j2 < Nv; j2++)
			{
				j2N = j2*Nv;
				for(j3 = 0; j3 < Nv; j3++)
				{
					k = iNNN + j1NN + j2N + j3;
					sum3 += (U[6*k] + U[6*k+5]/4.);
				}
			}
		}
	}
	sum3 = sum3*dx*x_diff;
	iNNN = ix*Nv*Nv*Nv;
	for(j1 = 0; j1 < Nv; j1++)
	{
		j1NN = j1*Nv*Nv;
		for(j2 = 0; j2 < Nv; j2++)
		{
			j2N = j2*Nv;
			for(j3 = 0; j3 < Nv; j3++)
			{
				k = iNNN + j1NN + j2N + j3;
				sum4 += U[6*k]*x_diff_sq/2. + U[6*k+1]*x_eval +  U[6*k+5]*x_diff_sq/8.;
			}
		}
	}

	C_E = computePhi_x_0(U);
	retn = (sum1 + sum3 + sum4)*dv*dv*dv - x*x/2 - C_E*x;
	return retn;
}

void PrintPhiVals(double *U, FILE *phifile)														// function to print the values of the potential and the density in the file tagged as phifile at the given timestep
{
	int i, np, nx, nv;																			// declare i (the index of the space cell), np (the number of points to evaluate in a given space/velocity cell), nx (a counter for the points in the space cell) & nv (a counter for the points in the velocity cell)
	double x_0, x_val, phi_val, rho_val, M_0, ddx;												// declare x_0 (the x value at the left edge of a given cell), x_val (the x value to be evaluated at), phi_val (the value of phi evaluated at x_val), rho_val (the value of rho evaluated at x_val) & ddx (the space between x values)

	np = 4;																						// set np to 4
	ddx = dx/np;																				// set ddx to the space cell width divided by np
	for(i=0; i<Nx; i++)
	{
		x_0 = Gridx((double)i - 0.5);															// set x_0 to the value of x at the left edge of the i-th space cell
		for (nx=0; nx<np; nx++)
		{
			x_val = x_0 + nx*ddx;																// set x_val to x_0 plus nx increments of width ddx

			phi_val = computePhi(U, x_val, i);													// calculate the value of phi, evaluated at x_val by using the function in the space cell
			rho_val = rho_x(x_val, U, i);														// calculate the value of rho, evaluated at x_val by using the function in the space cell
			M_0 = rho_val/(sqrt(1.8*PI));
			fprintf(phifile, "%11.8g %11.8g %11.8g \n", phi_val, rho_val, M_0);								// in the file tagged as phifile, print the value of the potential phi(t, x) and the density rho(t, x)
		}
	}
}

double computeC_rho(double *U, int i) // sum_m=0..i-1 int_{I_m} rho(z)dz   (Calculate integral of rho_h(z,t) from 0 to x as in eq. 53)
{
	double retn=0.;
	int j, k, m;
	for (m=0;m<i;m++){ //BUG: was "m < i-1"
		for(j=0;j<size_v;j++){
			k = m*size_v + j;
			retn += U[k*6+0] + U[k*6+5]/4.;
		}
	}
	
	retn *= dx*scalev;
	return retn;
}

double Int_Int_rho(double *U, int i) // \int_{I_i} [ \int^{x}_{x_i-0.5} rho(z)dz ] dx
{
  int j, k;
  double retn=0.;
  for(j=0;j<size_v;j++){
    k=i*size_v + j;
    retn += 0.5*(U[k*6+0] + U[k*6+5]/4.) - U[k*6+1]/12.;
  }
  
  return retn*dx*dx*scalev;  
}

double Int_Int_rho1st(double *U, int i)// \int_{I_i} [(x-x_i)/delta_x * \int^{x}_{x_i-0.5} rho(z)dz ] dx
{
 int j, k;
 double retn=0.;
 for(j=0;j<size_v;j++){
    k=i*size_v + j;
    retn += (U[k*6+0] + U[k*6+5]/4.)/12.;
  }
  return retn*dx*dx*scalev; 
}

/*double Int_Cumulativerho(double **U, int i)// \int_{I_i} [ \int^{x}_{0} rho(z)dz ] dx
{
  double retn=0., cp, tmp;
  cp = computeC_rho(U,i);
  tmp = Int_Int_rho(U,i);
  
  retn = dx*cp + tmp;
  return retn;  
}

double Int_Cumulativerho_sqr(double **U, int i)// \int_{I_i} [ \int^{x}_{0} rho(z)dz ]^2 dx
{
  double retn=0., cp, tmp1, tmp2, tmp3, c1=0., c2=0.;
  int j, k;
  cp = computeC_rho(U,i);
  tmp1 = cp*cp*dx;
  tmp2 = 2*cp*Int_Int_rho(U,i);
  for(j=0;j<size_v;j++){
    k=i*size_v + j;
    c1 += U[k][0] + U[k][5]/4.;
    c2 += U[k][1];
  }
  c2 *= dx/2.;
  tmp3 = pow(dv, 6)* ( c1*c1*dx*dx*dx/3. + c2*c2*dx/30. + c1*c2*(dx*Gridx((double)i)/6. - dx*dx/4.) );
  retn = tmp1 + tmp2 + tmp3;
  return retn;  
}*/

double Int_E(double *U, int i) // \int_i E dx      // Function to calculate the integral of E_h w.r.t. x over the interval I_i = [x_(i-1/2), x_(i+1/2))
{
	int m, j, k;
	double tmp=0., result;
	//#pragma omp parallel for shared(U) reduction(+:tmp)
	for(j=0;j<size_v;j++){
		for(m=0;m<i;m++){	
			k=m*size_v + j;
			tmp += U[k*6+0] + U[k*6+5]/4.;
		}
		k=i*size_v + j;
		tmp += 0.5*(U[k*6+0] + U[k*6+5]/4.) - U[k*6+1]/12.;		
	}

	//ce = computePhi_x_0(U);
	result = -ce*dx - tmp*dx*dx*scalev + Gridx((double)i)*dx;

	return result;
}

double Int_E1st(double *U, int i) // \int_i E*(x-x_i)/delta_x dx
{
	int j, k;
	double tmp=0., result;
	//#pragma omp parallel for reduction(+:tmp)
	for(j=0;j<size_v;j++){
		k=i*size_v + j;
		tmp += U[k*6+0] + U[k*6+5]/4.;
	}
	tmp = tmp*scalev;
	
	result = (1-tmp)*dx*dx/12.;

	return result;
}

double Int_fE(double *U, int i, int j) // \int f * E(f) dxdv on element I_i * K_j
{
	double retn=0.;
	int k;
	k = i*size_v + j;

	//retn = (U[k][0] + U[k][5]/4.)*Int_E(U,i) + U[k][1]*Int_E1st(U,i);
	retn = (U[k*6+0] + U[k*6+5]/4.)*intE[i] + U[k*6+1]*intE1[i];
	return retn*scalev;
}

double Int_E2nd(double *U, int i) // \int_i E* [(x-x_i)/delta_x]^2 dx
{
    int m, j, j1, j2, j3, k;
    double c1=0., c2=0., result;
  
    //cp = computeC_rho(U,i); ce = computePhi_x_0(U);

    for(j=0;j<size_v;j++){
	    k=i*size_v + j;
	    c1 += U[k*6+0] + U[k*6+5]/4.;
	    c2 += U[k*6+1];
    }
    c2 *= dx/2.;				
    
    result = (-cp[i] - ce+scalev*(c1*Gridx(i-0.5) + 0.25*c2))*dx/12. + (1-scalev*c1)*dx*Gridx((double)i)/12. - scalev*c2*dx/80.; //BUG: missed -cp

    return result;
}
