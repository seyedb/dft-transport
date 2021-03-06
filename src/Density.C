/*
Copyright (c) 2017 ETH Zurich
Sascha Brueck, Mauro Calderara, Mohammad Hossein Bani-Hashemian, and Mathieu Luisier

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <mpi.h>
#include <math.h>
#ifdef HAVE_SUPERLU
#include "SuperLU.H"
#endif
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits>
#include <algorithm>
#include "ScaLapack.H"
#include "tmprGF.H"
#include "FullInvert.H"
#include "LinearSolver.H"
#include "Full.H"
#include "Banded.H"
#ifdef HAVE_MUMPS
#include "MUMPS.H"
#endif
#include "Connection.H"
#ifdef HAVE_SPLITSOLVE
#include "SplitSolve.H"
#endif
#ifdef HAVE_PEXSI
#include "c_pexsi_interface.h"
#endif
#ifdef HAVE_PARDISO_SELINV
#include "Pardiso.H"
#endif
#include "GetSigma.H"
#include "Density.H"
#include <iostream>

/*! \brief Function that calls the inversion or linear solvers, adds up the LDOS computed from the solution to the P matrix, and computes the atom-resolved DOS and transmission for each energy point
*/
int density(TCSR<double> *KohnSham,TCSR<double> *Overlap,TCSR<double> *Ps,TCSR<double> *PsImag,CPX energy,CPX weight,CPX dweight,transport_methods::transport_method_type method,std::vector<double> muvec,std::vector<contact_type> contactvec,std::vector<result_type> &resultvec,std::vector<int> Bsizes,std::vector<int> orb_per_at,transport_parameters transport_params,MPI_Comm matrix_comm)
{
    double d_one = 1.0;
    double d_zer = 0.0;
    CPX z_one=CPX(d_one,d_zer);
    CPX z_zer=CPX(d_zer,d_zer);
    int matrix_procs,matrix_rank;
    MPI_Comm_size(matrix_comm,&matrix_procs);
    MPI_Comm_rank(matrix_comm,&matrix_rank);
double sabtime;
int worldrank; MPI_Comm_rank(MPI_COMM_WORLD,&worldrank);
    int n_mu=muvec.size();
    int GPUS_per_point=transport_params.gpus_per_point;
    bool run_splitsolve = transport_params.lin_solver_method==lin_solver_methods::SPLITSOLVE && method==transport_methods::WF;
int bandwidth=contactvec[0].bandwidth;
int ndof=contactvec[0].ndof;
int ncells=Overlap->size_tot/ndof;
int ntriblock=bandwidth*ndof;
int tra_block=0;
    TCSR<CPX> *HamSig = NULL;
    double *M_host = NULL;
    int Bsize = 0;
    std::vector<int> Bmin;
    std::vector<int> Bmax;
    std::vector<BoundarySelfEnergy> selfenergies(n_mu);
    int boundary_id=matrix_rank*n_mu/matrix_procs;
    if (run_splitsolve) boundary_id=(get_cpu_color(matrix_rank,1,matrix_procs,GPUS_per_point)+n_mu)%(n_mu+1);
    MPI_Comm boundary_comm;
    int key=matrix_rank;
    if (boundary_id<n_mu) key*=contactvec[boundary_id].inj_sign;
    MPI_Comm_split(matrix_comm,boundary_id,key,&boundary_comm);
    if (method!=transport_methods::EQ) {
sabtime=get_time(d_zer);
        TCSR<CPX> *SumHamC = new TCSR<CPX>(Overlap->size,Overlap->n_nonzeros,Overlap->findx);
        SumHamC->copy_contain(Overlap,d_one);
        c_zscal(SumHamC->n_nonzeros,-energy,SumHamC->nnz,1);
        c_daxpy(SumHamC->n_nonzeros,d_one,KohnSham->nnz,1,(double*)SumHamC->nnz,2);
if (!worldrank) cout << "TIME FOR SumHamC " << get_time(sabtime) << endl;
        if (!transport_params.cutl && !transport_params.cutr) {
            SumHamC->removepbc(bandwidth,ndof);
        }
// compute self energies
        if (run_splitsolve) {
            MPI_Comm dist_comm;
            int dist_size = matrix_procs/GPUS_per_point;
            int dist_color = matrix_rank/dist_size;
            MPI_Comm_split(matrix_comm,dist_color,matrix_rank,&dist_comm);
            int dist_root = dist_size-1;
            if (dist_color%2) dist_root = 0;
            HamSig = new TCSR<CPX>(SumHamC,dist_root,dist_comm);
            MPI_Comm_free(&dist_comm);
        }
        int n_bound_comm=min(matrix_procs,n_mu);
        for (int iseq=0;iseq<n_mu/n_bound_comm;iseq++) {
            int ipos=boundary_id+iseq*n_bound_comm;
            if (ipos<n_mu) {
                if ( selfenergies[ipos].Set_master(matrix_comm,boundary_comm) ) return (LOGCERR, EXIT_FAILURE);
            }
sabtime=get_time(d_zer);
            for (int i_bound_id=0;i_bound_id<n_bound_comm;i_bound_id++) {
                int ibpos=i_bound_id+iseq*n_bound_comm;
                if ( selfenergies[ibpos].Cutout(SumHamC,contactvec[ibpos],energy,method,matrix_comm) ) return (LOGCERR, EXIT_FAILURE);
            }
if (!worldrank) cout << "TIME FOR SIGMA CUTOUT " << get_time(sabtime) << endl;
sabtime=get_time(d_zer);
            if (ipos<n_mu) {
                if ( selfenergies[ipos].GetSigma(boundary_comm,transport_params) ) return (LOGCERR, EXIT_FAILURE);
            }
if (!worldrank) cout << "TIME FOR SIGMA GETSIGMA " << get_time(sabtime) << endl;
int left_gpu_rank  = ceil((double)matrix_procs/GPUS_per_point)-1;
if (worldrank==left_gpu_rank) cout << "TIME FOR FALL-THROUGH GETSIGMA " << get_time(sabtime) << endl;
            if (!run_splitsolve) {
MPI_Barrier(matrix_comm);
sabtime=get_time(d_zer);
                for (int i_bound_id=0;i_bound_id<n_bound_comm;i_bound_id++) {
                    int ibpos=i_bound_id+iseq*n_bound_comm;
                    selfenergies[ibpos].Distribute(SumHamC,matrix_comm);
                }
MPI_Barrier(matrix_comm);
if (!worldrank) cout << "TIME FOR SIGMA DISTRIBUTE " << get_time(sabtime) << endl;
            }
        }
//add sigma to sumhamc
        if (!run_splitsolve) {
            for (uint i_mu=0;i_mu<muvec.size();i_mu++) {
                resultvec[i_mu].npro=selfenergies[i_mu].n_propagating;
                resultvec[i_mu].ndec=selfenergies[i_mu].n_dec;
                resultvec[i_mu].eigval_degeneracy=selfenergies[i_mu].eigval_degeneracy;
                resultvec[i_mu].rcond=selfenergies[i_mu].rcond;
            }
sabtime=get_time(d_zer);
            TCSR<CPX> **HamSigVec = new TCSR<CPX>*[n_mu+1];
            CPX *HamSigSigns = new CPX[n_mu+1];
            HamSigVec[0]=SumHamC;
            HamSigSigns[0]=z_one;
            for (int i_mu=0;i_mu<n_mu;i_mu++) {
                HamSigVec[i_mu+1]=selfenergies[i_mu].spsigmadist;
                HamSigSigns[i_mu+1]=-z_one;
            }
            HamSig = new TCSR<CPX>(n_mu+1,HamSigSigns,HamSigVec);
            delete[] HamSigVec;
            delete[] HamSigSigns;
            for (int i_mu=0;i_mu<n_mu;i_mu++) selfenergies[i_mu].Deallocate_Sigma();
if (!worldrank) cout << "TIME FOR ADDING SIGMA " << get_time(sabtime) << endl;
        }
        delete SumHamC;
    }
    if (method==transport_methods::EQ) {
sabtime=get_time(d_zer);
        if (transport_params.inv_solver_method==inv_solver_methods::FULL) {
            TCSR<CPX> *SumHamC = new TCSR<CPX>(Overlap->size,Overlap->n_nonzeros,Overlap->findx);
            SumHamC->copy_contain(Overlap,d_one);
            c_zscal(SumHamC->n_nonzeros,-energy,SumHamC->nnz,1);
            c_daxpy(SumHamC->n_nonzeros,d_one,KohnSham->nnz,1,(double*)SumHamC->nnz,2);
            FullInvert solver(SumHamC,Ps,-weight/M_PI*CPX(0.0,1.0),matrix_comm);
            delete SumHamC;
#ifdef HAVE_PARDISO_SELINV
        } else if (transport_params.inv_solver_method==inv_solver_methods::PARDISO) {
            if (KohnSham->findx!=1 || Overlap->findx!=1) return (LOGCERR, EXIT_FAILURE);
            TCSR<CPX> *SumHamC = new TCSR<CPX>(Overlap->size,Overlap->n_nonzeros,Overlap->findx);
            SumHamC->copy_contain(Overlap,d_one);
            c_zscal(SumHamC->n_nonzeros,-energy,SumHamC->nnz,1);
            c_daxpy(SumHamC->n_nonzeros,d_one,KohnSham->nnz,1,(double*)SumHamC->nnz,2);
            if (!matrix_rank) {
                Pardiso::sparse_invert(SumHamC);
            }
            c_zscal(Overlap->n_nonzeros,-weight/M_PI*CPX(0.0,1.0),SumHamC->nnz,1);
            c_daxpy(Overlap->n_nonzeros,1.0,(double*)SumHamC->nnz,2,Ps->nnz,1);
            delete SumHamC;
#endif
#ifdef HAVE_PEXSI
        } else if (transport_params.inv_solver_method==inv_solver_methods::PEXSI) {
            if (KohnSham->findx!=1 || Overlap->findx!=1) return (LOGCERR, EXIT_FAILURE);
  
            double *HS_nnz_inp = new double[2*Overlap->n_nonzeros]();
            double *HS_nnz_out = new double[2*Overlap->n_nonzeros]();
  
            c_dcopy(Overlap->n_nonzeros,Overlap->nnz,1,HS_nnz_inp,2);
            c_zscal(Overlap->n_nonzeros,-energy,(CPX*)HS_nnz_inp,1);
            c_daxpy(Overlap->n_nonzeros,1.0,KohnSham->nnz,1,HS_nnz_inp,2);
  
            int n_nonzeros_global;
            MPI_Allreduce(&Overlap->n_nonzeros,&n_nonzeros_global,1,MPI_INT,MPI_SUM,matrix_comm);
            int info;
  
            PPEXSIOptions options;
            PPEXSISetDefaultOptions(&options);
            options.npSymbFact  = transport_params.pexsi_np_symb_fact;
            options.ordering    = transport_params.pexsi_ordering;
            options.rowOrdering = transport_params.pexsi_row_ordering;
            options.verbosity   = transport_params.pexsi_verbosity;
            int output_index    = -1;
            if (transport_params.pexsi_verbosity) output_index = worldrank;
      
            PPEXSIPlan plan;
            int nprowcol[2]={0,0};
            MPI_Dims_create(matrix_procs,2,nprowcol);
            plan = PPEXSIPlanInitialize(matrix_comm,nprowcol[0],nprowcol[1],output_index,&info);
            if (info) return (LOGCERR, EXIT_FAILURE);
            PPEXSILoadRealHSMatrix(plan,options,Overlap->size_tot,n_nonzeros_global,Overlap->n_nonzeros,Overlap->size,Overlap->edge_i,Overlap->index_j,HS_nnz_inp,1,NULL,&info);
            if (info) return (LOGCERR, EXIT_FAILURE);
            PPEXSISymbolicFactorizeComplexSymmetricMatrix(plan,options,&info);
            if (info) return (LOGCERR, EXIT_FAILURE);
            PPEXSISelInvComplexSymmetricMatrix(plan,options,HS_nnz_inp,HS_nnz_out,&info);
            if (info) return (LOGCERR, EXIT_FAILURE);
            PPEXSIPlanFinalize(plan,&info);
            if (info) return (LOGCERR, EXIT_FAILURE);
  
            delete[] HS_nnz_inp;
            c_zscal(Overlap->n_nonzeros,-weight/M_PI*CPX(0.0,1.0),(CPX*)HS_nnz_out,1);
            c_daxpy(Overlap->n_nonzeros,1.0,HS_nnz_out,2,Ps->nnz,1);
            delete[] HS_nnz_out;
#endif
        } else return (LOGCERR, EXIT_FAILURE);
if (!worldrank) cout << "TIME FOR EQ INVERSION " << get_time(sabtime) << endl;
    } else if (method==transport_methods::GF) {
sabtime=get_time(d_zer);
        if (transport_params.inv_solver_method==inv_solver_methods::FULL) {
            FullInvert solver(HamSig,Ps,-weight/M_PI*CPX(0.0,1.0),matrix_comm);
            if (transport_params.get_fermi_neutral) {
                FullInvert solverD(HamSig,PsImag,-dweight/M_PI*CPX(0.0,1.0),matrix_comm);
            }
            delete HamSig;
        } else if (transport_params.inv_solver_method==inv_solver_methods::RGF) {
            Bmax.push_back(Bsizes[0]-1);
            for (uint i=1;i<Bsizes.size();i++) Bmax.push_back(Bmax[i-1]+Bsizes[i]);
            Bmin.push_back(0);
            for (uint i=1;i<Bsizes.size();i++) Bmin.push_back(Bmax[i-1]+1);
            std::vector<int> Fsizes;
            for (uint i=0;i<Bsizes.size();i++) Fsizes.push_back(orb_per_at[Bmin[i]+Bsizes[i]]-orb_per_at[Bmin[i]]);
            if (!matrix_rank) {
                tmprGF::sparse_invert(HamSig,Fsizes);
            }
            Ps->add_real(HamSig,-weight/M_PI*CPX(0.0,1.0));
            if (transport_params.get_fermi_neutral) {
                PsImag->add_real(HamSig,-dweight/M_PI*CPX(0.0,1.0));
            }
            delete HamSig;
#ifdef HAVE_PARDISO_SELINV
        } else if (transport_params.inv_solver_method==inv_solver_methods::PARDISO) {
            if (HamSig->findx!=1) return (LOGCERR, EXIT_FAILURE);
            if (!matrix_rank) {
                Pardiso::sparse_invert(HamSig);
            }
            Ps->add_real(HamSig,-weight/M_PI*CPX(0.0,1.0));
            if (transport_params.get_fermi_neutral) {
                PsImag->add_real(HamSig,-dweight/M_PI*CPX(0.0,1.0));
            }
            delete HamSig;
#endif
#ifdef HAVE_PEXSI
        } else if (transport_params.inv_solver_method==inv_solver_methods::PEXSI) {
            if (HamSig->findx!=1) return (LOGCERR, EXIT_FAILURE);
  
            double *HS_nnz_inp = new double[2*HamSig->n_nonzeros]();
            c_dcopy(2*HamSig->n_nonzeros,(double*)HamSig->nnz,1,HS_nnz_inp,1);
  
            int n_nonzeros_global;
            MPI_Allreduce(&HamSig->n_nonzeros,&n_nonzeros_global,1,MPI_INT,MPI_SUM,matrix_comm);
            int info;
  
            PPEXSIOptions options;
            PPEXSISetDefaultOptions(&options);
            options.npSymbFact  = transport_params.pexsi_np_symb_fact;
            options.ordering    = transport_params.pexsi_ordering;
            options.rowOrdering = transport_params.pexsi_row_ordering;
            options.verbosity   = transport_params.pexsi_verbosity;
            int output_index    = -1;
            if (transport_params.pexsi_verbosity) output_index = worldrank;
      
            PPEXSIPlan plan;
            int nprowcol[2]={0,0};
            MPI_Dims_create(matrix_procs,2,nprowcol);
            plan = PPEXSIPlanInitialize(matrix_comm,nprowcol[0],nprowcol[1],output_index,&info);
            if (info) return (LOGCERR, EXIT_FAILURE);
            PPEXSILoadRealHSMatrix(plan,options,HamSig->size_tot,n_nonzeros_global,HamSig->n_nonzeros,HamSig->size,HamSig->edge_i,HamSig->index_j,HS_nnz_inp,1,NULL,&info);
            if (info) return (LOGCERR, EXIT_FAILURE);
            PPEXSISymbolicFactorizeComplexSymmetricMatrix(plan,options,&info);
            if (info) return (LOGCERR, EXIT_FAILURE);
            PPEXSISelInvComplexSymmetricMatrix(plan,options,HS_nnz_inp,(double*)HamSig->nnz,&info);
            if (info) return (LOGCERR, EXIT_FAILURE);
            PPEXSIPlanFinalize(plan,&info);
            if (info) return (LOGCERR, EXIT_FAILURE);
  
            delete[] HS_nnz_inp;
            Ps->add_real(HamSig,-weight/M_PI*CPX(0.0,1.0));
            if (transport_params.get_fermi_neutral) {
                PsImag->add_real(HamSig,-dweight/M_PI*CPX(0.0,1.0));
            }
            delete HamSig;
#endif
        } else return (LOGCERR, EXIT_FAILURE);
if (!worldrank) cout << "TIME FOR GF INVERSION " << get_time(sabtime) << endl;
    } else if (method==transport_methods::NEGF) {
sabtime=get_time(d_zer);
        LinearSolver<CPX>* solver;
        if (transport_params.lin_solver_method==lin_solver_methods::FULL) {
            solver = new Full<CPX>(HamSig,matrix_comm);
#ifdef HAVE_SUPERLU
        } else if (transport_params.lin_solver_method==lin_solver_methods::SUPERLU) {
            solver = new SuperLU<CPX>(HamSig,matrix_comm);
#endif
#ifdef HAVE_MUMPS
        } else if (transport_params.lin_solver_method==lin_solver_methods::MUMPS) {
            solver = new MUMPS<CPX>(HamSig,matrix_comm);
#endif
        } else if (transport_params.lin_solver_method==lin_solver_methods::BANDED) {
            solver = new Banded<CPX>(HamSig,matrix_comm);
        } else return (LOGCERR, EXIT_FAILURE);
        solver->prepare(&Bmin[0],&Bmax[0],Bmin.size(),Bsize,&orb_per_at[0],10);
if (!worldrank) cout << "TIME FOR NEGF SPARSE DECOMPOSITION PHASE " << get_time(sabtime) << endl;
sabtime=get_time(d_zer);
        CPX* inj = NULL;
        CPX* sol = NULL;
        int *dist_sol = NULL;
        int *displc_sol = NULL;
        int nprol = contactvec[0].ndof*contactvec[0].bandwidth;
        int npror = contactvec[1].ndof*contactvec[1].bandwidth;
        int sigmastartl = contactvec[0].start;
        if (contactvec[0].inj_sign==-1) sigmastartl+=-(contactvec[0].bandwidth-1)*contactvec[0].ndof;
        int sigmastartr = contactvec[1].start;
        if (contactvec[1].inj_sign==-1) sigmastartr+=-(contactvec[1].bandwidth-1)*contactvec[1].ndof;
        dist_sol = new int[matrix_procs];
        MPI_Allgather(&HamSig->size,1,MPI_INT,dist_sol,1,MPI_INT,matrix_comm);
        displc_sol = new int[matrix_procs+1]();
        for (int iii=1;iii<matrix_procs+1;iii++) {
            displc_sol[iii]=displc_sol[iii-1]+dist_sol[iii-1];
        }
        int injsize_loc_max=*max_element(dist_sol,dist_sol+matrix_procs);
        inj = new CPX[injsize_loc_max*(nprol+npror)]();
        for (int i=0;i<nprol;i++) {
            int iloc=i+sigmastartl-displc_sol[matrix_rank];
            if (iloc>=0 && iloc<dist_sol[matrix_rank]) {
                inj[iloc+i*dist_sol[matrix_rank]]=1.0;
            }
        }
        for (int i=0;i<npror;i++) {
            int iloc=i+sigmastartr-displc_sol[matrix_rank];
            if (iloc>=0 && iloc<dist_sol[matrix_rank]) {
                inj[iloc+i*dist_sol[matrix_rank]+nprol*dist_sol[matrix_rank]]=1.0;
            }
        }
        sol = new CPX[dist_sol[matrix_rank]*(nprol+npror)]();
        solver->solve_equation(sol, inj, nprol+npror);
        delete solver;
        delete HamSig;
if (!worldrank) cout << "TIME FOR NEGF SPARSE SOLVE PHASE " << get_time(sabtime) << endl;
        int solsize=Ps->size_tot;
        CPX* Sol = new CPX[solsize*(nprol+npror)];
        for (int icol=0;icol<nprol+npror;icol++) {
            MPI_Allgatherv(&sol[dist_sol[matrix_rank]*icol],dist_sol[matrix_rank],MPI_DOUBLE_COMPLEX,&Sol[solsize*icol],dist_sol,displc_sol,MPI_DOUBLE_COMPLEX,matrix_comm);
        }
sabtime=get_time(d_zer);
        c_zgemm('N','N',dist_sol[matrix_rank],nprol,nprol,z_one,sol,dist_sol[matrix_rank],selfenergies[0].gamma,nprol,z_zer,inj,dist_sol[matrix_rank]);
        c_zgemm('N','N',dist_sol[matrix_rank],npror,npror,z_one,&sol[dist_sol[matrix_rank]*nprol],dist_sol[matrix_rank],selfenergies[1].gamma,npror,z_zer,&inj[dist_sol[matrix_rank]*nprol],dist_sol[matrix_rank]);
if (!worldrank) cout << "TIME FOR GAMMA MULTIPLICATION " << get_time(sabtime) << endl;
sabtime=get_time(d_zer);
        full_transpose(nprol,dist_sol[matrix_rank],inj,sol);
        full_transpose(npror,dist_sol[matrix_rank],&inj[dist_sol[matrix_rank]*nprol],&sol[dist_sol[matrix_rank]*nprol]);
        delete[] inj;
        CPX* SolT = new CPX[solsize*max(nprol,npror)];
        if (transport_params.cp2k_method==cp2k_methods::TRANSPORT) {
            if (muvec[0]>muvec[1]) {
                double fermil = fermi(real(energy),muvec[0],transport_params.temperature,0)-fermi(real(energy),muvec[1],transport_params.temperature,0);
                full_transpose(nprol,solsize,Sol,SolT);
                Ps->psipsidagger_transpose(sol,SolT,nprol,-weight/2.0/M_PI*fermil);
            } else {
                double fermir = fermi(real(energy),muvec[1],transport_params.temperature,0)-fermi(real(energy),muvec[0],transport_params.temperature,0);
                full_transpose(npror,solsize,&Sol[solsize*nprol],SolT);
                Ps->psipsidagger_transpose(&sol[dist_sol[matrix_rank]*nprol],SolT,npror,-weight/2.0/M_PI*fermir);
            }
        }
if (!worldrank) cout << "TIME FOR CONSTRUCTION OF S-PATTERN DENSITY MATRIX " << get_time(sabtime) << endl;
sabtime=get_time(d_zer);
        full_transpose(nprol,solsize,Sol,SolT);
        for (int i=0;i<npror;i++) {
            int iloc=i+sigmastartr-displc_sol[matrix_rank];
            for (int j=0;j<npror;j++) {
                if (iloc>=0 && iloc<dist_sol[matrix_rank]) {
                    selfenergies[1].gamma[i+j*npror]*=c_zdotc(nprol,&SolT[(j+sigmastartr)*nprol],1,&sol[iloc*nprol],1);
                } else {
                    selfenergies[1].gamma[i+j*npror]=z_zer;
                }
            }
        }
        full_transpose(npror,solsize,&Sol[solsize*nprol],SolT);
        for (int i=0;i<nprol;i++) {
            int iloc=i+sigmastartl-displc_sol[matrix_rank];
            for (int j=0;j<nprol;j++) {
                if (iloc>=0 && iloc<dist_sol[matrix_rank]) {
                    selfenergies[0].gamma[i+j*nprol]*=c_zdotc(npror,&SolT[(j+sigmastartl)*npror],1,&sol[iloc*npror+dist_sol[matrix_rank]*nprol],1);
                } else {
                    selfenergies[0].gamma[i+j*nprol]=z_zer;
                }
            }
        }
        delete[] dist_sol;
        delete[] displc_sol;
        delete[] SolT;
        delete[] sol;
        delete[] Sol;
        CPX transml_loc = std::accumulate(selfenergies[1].gamma,selfenergies[1].gamma+npror*npror,z_zer);
        CPX transmr_loc = std::accumulate(selfenergies[0].gamma,selfenergies[0].gamma+nprol*nprol,z_zer);
        double transml;
        double transmr;
        MPI_Allreduce(&transml_loc,&transml,1,MPI_DOUBLE,MPI_SUM,matrix_comm);
        MPI_Allreduce(&transmr_loc,&transmr,1,MPI_DOUBLE,MPI_SUM,matrix_comm);
if (!worldrank) cout << "TIME FOR TRANSMISSION " << get_time(sabtime) << endl;
        if (!matrix_rank) {
            resultvec[0].transm=transml;
            resultvec[1].transm=transmr;
        }
        for (int i_mu=0;i_mu<n_mu;i_mu++) selfenergies[i_mu].Deallocate_Gamma();
    } else if (method==transport_methods::WF) {
        TCSR<CPX> *H1cut = new TCSR<CPX>(HamSig,tra_block*ntriblock,ntriblock,(tra_block+1)*ntriblock,ntriblock);
        TCSR<CPX> *H1 = new TCSR<CPX>(H1cut,0,matrix_comm);
        delete H1cut;
sabtime=get_time(d_zer);
        LinearSolver<CPX>* solver;
        if (transport_params.lin_solver_method==lin_solver_methods::FULL) {
            solver = new Full<CPX>(HamSig,matrix_comm);
#ifdef HAVE_SUPERLU
        } else if (transport_params.lin_solver_method==lin_solver_methods::SUPERLU) {
            solver = new SuperLU<CPX>(HamSig,matrix_comm);
#endif
#ifdef HAVE_MUMPS
        } else if (transport_params.lin_solver_method==lin_solver_methods::MUMPS) {
            solver = new MUMPS<CPX>(HamSig,matrix_comm);
#endif
        } else if (transport_params.lin_solver_method==lin_solver_methods::BANDED) {
            solver = new Banded<CPX>(HamSig,matrix_comm);
#ifdef HAVE_SPLITSOLVE
        } else if (transport_params.lin_solver_method==lin_solver_methods::SPLITSOLVE) {
            Bmax.push_back(Bsizes[0]-1);
            for (uint i=1;i<Bsizes.size();i++) Bmax.push_back(Bmax[i-1]+Bsizes[i]);
            Bmin.push_back(0);
            for (uint i=1;i<Bsizes.size();i++) Bmin.push_back(Bmax[i-1]+1);
            std::vector<int> Fsizes;
            for (uint i=0;i<Bsizes.size();i++) Fsizes.push_back(orb_per_at[Bmin[i]+Bsizes[i]]-orb_per_at[Bmin[i]]);
            Bsize=*std::max_element(Fsizes.begin(),Fsizes.end());
            if (boundary_id==n_mu) {
                if (cudaMallocHost((void **)&M_host,(HamSig->size+Bsize)*Bsize*sizeof(double)+5*Bsize*Bsize*sizeof(CPX))!=cudaSuccess) return (LOGCERR, EXIT_FAILURE);
            }
            solver = new SplitSolve<double>(HamSig,M_host,matrix_procs-GPUS_per_point,1,boundary_comm,matrix_comm);
#endif
        } else return (LOGCERR, EXIT_FAILURE);
        solver->prepare(&Bmin[0],&Bmax[0],Bmin.size(),Bsize,&orb_per_at[0],10);
        CPX* inj = NULL;
        CPX* sol = NULL;
        int nprol = 0;
        int npror = 0;
        CPX *lambda = NULL;
        int *dist_sol = NULL;
        int *displc_sol = NULL;
        int left_gpu_rank  = ceil((double)matrix_procs/GPUS_per_point)-1;
        int right_gpu_rank = matrix_procs-ceil((double)matrix_procs/GPUS_per_point);
if (worldrank==left_gpu_rank) cout << "TIME FOR WAVEFUNCTION SPARSE DECOMPOSITION PHASE " << get_time(sabtime) << endl;
if (!worldrank) cout << "TIME FOR FALL-THROUGH RANK WAVEFUNCTION " << get_time(sabtime) << endl;
MPI_Barrier(matrix_comm);
        if (transport_params.lin_solver_method==lin_solver_methods::SPLITSOLVE) {
            int left_bc_rank  = 0;
            int right_bc_rank = matrix_procs-1;
            int NBC[2]={contactvec[0].ndof*contactvec[0].bandwidth,contactvec[1].ndof*contactvec[1].bandwidth};
            solver->prepare_corner(selfenergies[0].sigma,selfenergies[1].sigma,NBC,NULL,NULL,0,0,NULL,0);
            if (matrix_rank==left_bc_rank) nprol=selfenergies[0].n_propagating;
            MPI_Bcast(&nprol,1,MPI_INT,left_bc_rank,matrix_comm);
            if (matrix_rank==right_bc_rank) npror=selfenergies[1].n_propagating;
            MPI_Bcast(&npror,1,MPI_INT,right_bc_rank,matrix_comm);
            lambda = new CPX[nprol+npror];
            if (matrix_rank==left_bc_rank) c_zcopy(nprol,selfenergies[0].lambdapro,1,lambda,1);
            MPI_Bcast(lambda,nprol,MPI_DOUBLE_COMPLEX,left_bc_rank,matrix_comm);
            if (matrix_rank==right_bc_rank) c_zcopy(npror,selfenergies[1].lambdapro,1,&lambda[nprol],1);
            MPI_Bcast(&lambda[nprol],npror,MPI_DOUBLE_COMPLEX,right_bc_rank,matrix_comm);
            MPI_Status status;
            if (matrix_rank==left_bc_rank) MPI_Send(selfenergies[0].inj,NBC[0]*nprol,MPI_DOUBLE_COMPLEX,left_gpu_rank,1,matrix_comm);
            if (matrix_rank==left_gpu_rank) {
                inj = new CPX[NBC[0]*(nprol+npror)]();
                MPI_Recv(inj,NBC[0]*nprol,MPI_DOUBLE_COMPLEX,left_bc_rank,1,matrix_comm,&status);
            }
            if (matrix_rank==right_bc_rank) MPI_Send(selfenergies[1].inj,NBC[1]*npror,MPI_DOUBLE_COMPLEX,right_gpu_rank,3,matrix_comm);
            if (matrix_rank==right_gpu_rank) {
                inj = new CPX[NBC[1]*(nprol+npror)]();
                MPI_Recv(&inj[NBC[1]*nprol],NBC[1]*npror,MPI_DOUBLE_COMPLEX,right_bc_rank,3,matrix_comm,&status);
            }
//if npror is zero do i get a segfault because i access the bad element or is it not accessed
            if (matrix_rank==left_bc_rank) {
                resultvec[0].npro=selfenergies[0].n_propagating;
                resultvec[0].ndec=selfenergies[0].n_dec;
                resultvec[0].eigval_degeneracy=selfenergies[0].eigval_degeneracy;
                resultvec[0].rcond=selfenergies[0].rcond;
                selfenergies[0].Finalize();
            }
            MPI_Bcast(&resultvec[0].npro,1,MPI_INT,left_bc_rank,matrix_comm);
            MPI_Bcast(&resultvec[0].ndec,1,MPI_INT,left_bc_rank,matrix_comm);
            MPI_Bcast(&resultvec[0].eigval_degeneracy,1,MPI_INT,left_bc_rank,matrix_comm);
            MPI_Bcast(&resultvec[0].rcond,1,MPI_DOUBLE,left_bc_rank,matrix_comm);
            if (matrix_rank==right_bc_rank) {
                resultvec[1].npro=selfenergies[1].n_propagating;
                resultvec[1].ndec=selfenergies[1].n_dec;
                resultvec[1].eigval_degeneracy=selfenergies[1].eigval_degeneracy;
                resultvec[1].rcond=selfenergies[1].rcond;
                selfenergies[1].Finalize();
            }
            MPI_Bcast(&resultvec[1].npro,1,MPI_INT,right_bc_rank,matrix_comm);
            MPI_Bcast(&resultvec[1].ndec,1,MPI_INT,right_bc_rank,matrix_comm);
            MPI_Bcast(&resultvec[1].eigval_degeneracy,1,MPI_INT,right_bc_rank,matrix_comm);
            MPI_Bcast(&resultvec[1].rcond,1,MPI_DOUBLE,right_bc_rank,matrix_comm);
            if (boundary_id==n_mu) {
                int solver_size,solver_rank;
                MPI_Comm_size(boundary_comm,&solver_size);
                MPI_Comm_rank(boundary_comm,&solver_rank);
                dist_sol = new int[solver_size];
                MPI_Allgather(&HamSig->size,1,MPI_INT,dist_sol,1,MPI_INT,boundary_comm);
                displc_sol = new int[solver_size+1]();
                for (int iii=1;iii<solver_size+1;iii++) {
                    displc_sol[iii]=displc_sol[iii-1]+dist_sol[iii-1];
                }
                sol = new CPX[dist_sol[solver_rank]*(nprol+npror)]();
            }
        } else {
            nprol=selfenergies[0].n_propagating;
            npror=selfenergies[1].n_propagating;
            lambda = new CPX[nprol+npror];
            c_zcopy(nprol,selfenergies[0].lambdapro,1,lambda,1);
            c_zcopy(npror,selfenergies[1].lambdapro,1,&lambda[nprol],1);
            dist_sol = new int[matrix_procs];
            MPI_Allgather(&HamSig->size,1,MPI_INT,dist_sol,1,MPI_INT,matrix_comm);
            displc_sol = new int[matrix_procs+1]();
            for (int iii=1;iii<matrix_procs+1;iii++) {
                displc_sol[iii]=displc_sol[iii-1]+dist_sol[iii-1];
            }
            int injsize_loc_max=*max_element(dist_sol,dist_sol+matrix_procs);
            inj = new CPX[injsize_loc_max*(nprol+npror)]();
            if (selfenergies[0].spainjdist->n_nonzeros) {
                selfenergies[0].spainjdist->sparse_to_full(inj,HamSig->size,nprol);
            }
            if (selfenergies[1].spainjdist->n_nonzeros) {
                selfenergies[1].spainjdist->sparse_to_full(&inj[HamSig->size*nprol],HamSig->size,npror);
            }
            for (int i_mu=0;i_mu<n_mu;i_mu++) selfenergies[i_mu].Deallocate_Injection();
            sol = new CPX[dist_sol[matrix_rank]*(nprol+npror)]();
        }
sabtime=get_time(d_zer);
        solver->solve_equation(sol, inj, nprol+npror);
        if (transport_params.lin_solver_method!=lin_solver_methods::SPLITSOLVE || matrix_rank==left_gpu_rank || matrix_rank==right_gpu_rank) {
            delete[] inj;
        }
        delete solver;
        delete HamSig;
#ifdef HAVE_SPLITSOLVE
        if (transport_params.lin_solver_method==lin_solver_methods::SPLITSOLVE && boundary_id==n_mu) {
            cudaFreeHost(M_host);
        }
#endif
if (worldrank==left_gpu_rank) cout << "TIME FOR WAVEFUNCTION SPARSE SOLVE PHASE " << get_time(sabtime) << endl;
        int solsize=Ps->size_tot;
        CPX* Sol = new CPX[solsize*(nprol+npror)];
        for (int icol=0;icol<nprol+npror;icol++) {
            if (transport_params.lin_solver_method==lin_solver_methods::SPLITSOLVE) {
                if (boundary_id==n_mu) {
                    int solver_rank;
                    MPI_Comm_rank(boundary_comm,&solver_rank);
                    MPI_Allgatherv(&sol[dist_sol[solver_rank]*icol],dist_sol[solver_rank],MPI_DOUBLE_COMPLEX,&Sol[solsize*icol],dist_sol,displc_sol,MPI_DOUBLE_COMPLEX,boundary_comm);
                }
                MPI_Bcast(&Sol[solsize*icol],solsize,MPI_DOUBLE_COMPLEX,left_gpu_rank,matrix_comm);
            } else {
                MPI_Allgatherv(&sol[dist_sol[matrix_rank]*icol],dist_sol[matrix_rank],MPI_DOUBLE_COMPLEX,&Sol[solsize*icol],dist_sol,displc_sol,MPI_DOUBLE_COMPLEX,matrix_comm);
            }
        }
        if (transport_params.lin_solver_method!=lin_solver_methods::SPLITSOLVE || boundary_id==n_mu) {
            delete[] sol;
            delete[] dist_sol;
            delete[] displc_sol;
        }
//maybe i should have a solver comm (pointer to MPI_COMM_NULL after use) and a function sol_alloc and sol_dealloc
        if (transport_params.cp2k_method==cp2k_methods::TRANSMISSION) {
            ifstream rangefile("CURRENT_CUBE_RANGE");
            CPX factor_w=0.0;
            if (weight==1.0) {
                factor_w=1.0;
            } else if (abs(muvec[0]-muvec[1])>0.01) {
                if (muvec[0]>muvec[1]) {
                    double fermil = fermi(real(energy),muvec[0],transport_params.temperature,0)-fermi(real(energy),muvec[1],transport_params.temperature,0);
                    factor_w=+weight*fermil;
                } else {
                    double fermir = fermi(real(energy),muvec[1],transport_params.temperature,0)-fermi(real(energy),muvec[0],transport_params.temperature,0);
                    factor_w=+weight*fermir;
                }
            } else if (rangefile) {
                double range_start, range_end;
                rangefile >> range_start;
                rangefile >> range_end;
                if (real(energy)>range_start && real(energy)<range_end) {
                    factor_w=weight;
                }
                rangefile.close();
            }
            CPX* SolT = new CPX[solsize*nprol];
            full_transpose(nprol,solsize,Sol,SolT);
            Ps->psipsidagger_transpose(Overlap,resultvec[0].dosprofile,&orb_per_at[0],PsImag,SolT,nprol,-0.5*factor_w,matrix_comm);
            delete[] SolT;
            SolT = new CPX[solsize*npror];
            full_transpose(npror,solsize,&Sol[solsize*nprol],SolT);
            Ps->psipsidagger_transpose(Overlap,resultvec[1].dosprofile,&orb_per_at[0],PsImag,SolT,npror,+0.5*factor_w,matrix_comm);
            delete[] SolT;
        }
        if (transport_params.cp2k_method==cp2k_methods::TRANSPORT) {
sabtime=get_time(d_zer);
//int minmuarg=std::min_element( vec.begin(), vec.end() ); and then loop over all that are not minmuarg
            if (muvec[0]>muvec[1]) {
                double fermil = fermi(real(energy),muvec[0],transport_params.temperature,0)-fermi(real(energy),muvec[1],transport_params.temperature,0);
                CPX* SolT = new CPX[solsize*nprol];
                full_transpose(nprol,solsize,Sol,SolT);
                if (transport_params.get_fermi_neutral) {
                    Overlap->psipsidagger_transpose(resultvec[0].dosprofile,SolT,nprol,matrix_comm);
                } else {
                    Ps->psipsidagger_transpose(Overlap,resultvec[0].dosprofile,&orb_per_at[0],PsImag,SolT,nprol,+weight*fermil,matrix_comm);
                    delete[] SolT;
                    SolT = new CPX[solsize*npror];
                    full_transpose(npror,solsize,&Sol[solsize*nprol],SolT);
                    Ps->psipsidagger_transpose(Overlap,resultvec[1].dosprofile,&orb_per_at[0],PsImag,SolT,npror,0.0,matrix_comm);
                }
                delete[] SolT;
            } else {
                double fermir = fermi(real(energy),muvec[1],transport_params.temperature,0)-fermi(real(energy),muvec[0],transport_params.temperature,0);
                CPX* SolT = new CPX[solsize*npror];
                full_transpose(npror,solsize,&Sol[solsize*nprol],SolT);
                if (transport_params.get_fermi_neutral) {
                    Overlap->psipsidagger_transpose(resultvec[1].dosprofile,SolT,npror,matrix_comm);
                } else {
                    Ps->psipsidagger_transpose(Overlap,resultvec[1].dosprofile,&orb_per_at[0],PsImag,SolT,npror,+weight*fermir,matrix_comm);
                    delete[] SolT;
                    SolT = new CPX[solsize*nprol];
                    full_transpose(nprol,solsize,Sol,SolT);
                    Ps->psipsidagger_transpose(Overlap,resultvec[0].dosprofile,&orb_per_at[0],PsImag,SolT,nprol,0.0,matrix_comm);
                }
                delete[] SolT;
            }
if (!worldrank) cout << "TIME FOR CONSTRUCTION OF S-PATTERN DENSITY MATRIX " << get_time(sabtime) << endl;
        }
        if (transport_params.cp2k_method==cp2k_methods::LOCAL_SCF) {
            CPX* Soll = new CPX[solsize*(nprol+npror)]();
            CPX* Solr = new CPX[solsize*(nprol+npror)]();
            for (int ibw=bandwidth+1;ibw<ncells;ibw++) {
//            for (int ibw=0;ibw<ncells;ibw++) {
                c_zlacpy('A',ndof,nprol+npror,Sol,solsize,&Soll[ndof*ibw],solsize);
                c_zlacpy('A',ndof,nprol+npror,&Sol[solsize-ndof],solsize,&Solr[solsize-ndof*(ibw+1)],solsize);
                for (int ipro=0;ipro<nprol+npror;ipro++) {
                    c_zscal(solsize,pow(lambda[ipro],-1),&Soll[ipro*solsize],1);
                    c_zscal(solsize,pow(lambda[ipro],+1),&Solr[ipro*solsize],1);
                }
            }
            double fermil=fermi(real(energy),muvec[0],transport_params.temperature,0);
            double fermir=fermi(real(energy),muvec[1],transport_params.temperature,0);
sabtime=get_time(d_zer);
            CPX* SolT = new CPX[solsize*max(nprol,npror)];
            CPX* SollT = new CPX[solsize*max(nprol,npror)];
            CPX* SolrT = new CPX[solsize*max(nprol,npror)];
            full_transpose(nprol,solsize,Sol,SolT);
            full_transpose(nprol,solsize,Soll,SollT);
            full_transpose(nprol,solsize,Solr,SolrT);
            resultvec[0].dos=Ps->psipsidagger_transpose(Overlap,SolT,SollT,SolrT,nprol,ndof,bandwidth,+weight*fermil);
            full_transpose(npror,solsize,&Sol[solsize*nprol],SolT);
            full_transpose(npror,solsize,&Soll[solsize*nprol],SollT);
            full_transpose(npror,solsize,&Solr[solsize*nprol],SolrT);
            resultvec[1].dos=Ps->psipsidagger_transpose(Overlap,SolT,SollT,SolrT,npror,ndof,bandwidth,+weight*fermir);
            delete[] SolT;
            delete[] SollT;
            delete[] SolrT;
/*
            Ps->psipsidagger(Overlap,Sol,Soll,Solr,nprol,ndof,bandwidth,+weight*fermil);
            Ps->psipsidagger(Overlap,&Sol[Ps->size_tot*nprol],&Soll[Ps->size_tot*nprol],&Solr[Ps->size_tot*nprol],npror,ndof,bandwidth,+weight*fermir);
*/
if (!worldrank) cout << "TIME FOR CONSTRUCTION OF S-PATTERN DENSITY MATRIX " << get_time(sabtime) << endl;
            delete[] Soll;
            delete[] Solr;
        }
        delete[] lambda;
// transmission
        if (!matrix_rank) {
            CPX *vecoutdof=new CPX[ntriblock];
            resultvec[0].transm=d_zer;
            for (int ipro=0;ipro<nprol;ipro++) {
                H1->mat_vec_mult(&Sol[Ps->size_tot*ipro+(tra_block+1)*ntriblock],vecoutdof,1);
                 resultvec[0].transm+=4*M_PI*imag(c_zdotc(ntriblock,&Sol[Ps->size_tot*ipro+tra_block*ntriblock],1,vecoutdof,1));
            }
            resultvec[1].transm=d_zer;
            for (int ipro=nprol;ipro<nprol+npror;ipro++) {
                H1->mat_vec_mult(&Sol[Ps->size_tot*ipro+(tra_block+1)*ntriblock],vecoutdof,1);
                resultvec[1].transm+=4*M_PI*imag(c_zdotc(ntriblock,&Sol[Ps->size_tot*ipro+tra_block*ntriblock],1,vecoutdof,1));
            }
            delete[] vecoutdof;
        }
        delete[] Sol;
        delete H1;
    } else return (LOGCERR, EXIT_FAILURE);
    MPI_Comm_free(&boundary_comm);

    return 0;
}
