#include "FEM.h"

void FEM::MatAssyUSNS(MatrixXd &Klocal, VectorXd &Flocal, const int ic, const int t_itr)
{
  int ii, jj, mm, nn, s, t, u;
  int IU,IV,IW,IP;
  int JU,JV,JW,JP;

  int GP = 3;
  
  VDOUBLE2D x_current(numOfNodeInElm,VDOUBLE1D(3,0e0));
  
  VDOUBLE1D N(numOfNodeInElm,0);
  VDOUBLE2D dNdr(numOfNodeInElm,VDOUBLE1D(3,0e0));
  VDOUBLE2D dNdx(numOfNodeInElm,VDOUBLE1D(3,0e0));

  VDOUBLE2D K(numOfNodeInElm,VDOUBLE1D(numOfNodeInElm,0e0));
  
  for(int i=0;i<numOfNodeInElm;i++){
    for(int j=0;j<3;j++){
      x_current[i][j] = xFluid[elmFluid[ic]->nodeNumsPrevFluid[i]][j];
    }
  }

  double dxdr[3][3];
  double detJ, weight;
  
  Gauss gauss(GP); 
  
  for(int i1=0; i1<GP; i1++){
    for(int i2=0; i2<GP; i2++){
      for(int i3=0; i3<GP; i3++){
        
        ShapeFunction3D::C3D8_N(N,gauss.point[i1],gauss.point[i2],gauss.point[i3]);
        ShapeFunction3D::C3D8_dNdr(dNdr,gauss.point[i1],gauss.point[i2],gauss.point[i3]);
        
        MathFEM::calc_dxdr(dxdr,dNdr,x_current,numOfNodeInElm);
            
        detJ = dxdr[0][0]*dxdr[1][1]*dxdr[2][2]+dxdr[0][1]*dxdr[1][2]*dxdr[2][0]+dxdr[0][2]*dxdr[1][0]*dxdr[2][1]
              -dxdr[0][2]*dxdr[1][1]*dxdr[2][0]-dxdr[0][1]*dxdr[1][0]*dxdr[2][2]-dxdr[0][0]*dxdr[1][2]*dxdr[2][1];
        weight = gauss.weight[i1] * gauss.weight[i2] * gauss.weight[i3];
              
        MathFEM::calc_dNdx(dNdx,dNdr,dxdr,numOfNodeInElm);
        MathFEM::calc_dNdx(dNdx,dNdr,dxdr,numOfNodeInElm);

        double vel[3]={0e0,0e0,0e0};
        double advel[3]={0e0,0e0,0e0};
        double dvdx[3][3];

        velocityValue(vel,advel,dvdx,N,dNdx,ic,t_itr);

        double tau = calc_tau2(advel);
        
        for(ii=0;ii<numOfNodeInElm;ii++)
        {  
          IU = 4*ii;
          IV = IU+1;
          IW = IU+2;
          IP = IU+3;
          for(jj=0;jj<numOfNodeInElm;jj++)
          {  
            JU = 4*jj;
            JV = JU+1;
            JW = JU+2;
            JP = JU+3;

            K[ii][jj] = 0e0;
            
            for(int k=0;k<3;k++){
              K[ii][jj] += dNdx[ii][k]*dNdx[jj][k];
            }

            // MASS TERM
            Klocal(IU, JU) += N[ii] * N[jj] / dt * detJ * weight;
            Klocal(IV, JV) += N[ii] * N[jj] / dt * detJ * weight;
            Klocal(IW, JW) += N[ii] * N[jj] / dt * detJ * weight;

            // DIFFUSION TERM
            for(mm=0;mm<3;mm++)
            {
              if(mm == 0){s = 2e0; t = 1e0; u = 1e0;}
              if(mm == 1){s = 1e0; t = 2e0; u = 1e0;}
              if(mm == 2){s = 1e0; t = 1e0; u = 2e0;}

              Klocal(IU, JU) += 5e-1 * s * dNdx[ii][mm] * dNdx[jj][mm] / Re * detJ * weight;
              Klocal(IV, JV) += 5e-1 * t * dNdx[ii][mm] * dNdx[jj][mm] / Re * detJ * weight;
              Klocal(IW, JW) += 5e-1 * u * dNdx[ii][mm] * dNdx[jj][mm] / Re * detJ * weight;
            }
            Klocal(IU, JV) += 5e-1 * dNdx[ii][1] * dNdx[jj][0] / Re * detJ * weight;
            Klocal(IU, JW) += 5e-1 * dNdx[ii][2] * dNdx[jj][0] / Re * detJ * weight;
            Klocal(IV, JU) += 5e-1 * dNdx[ii][0] * dNdx[jj][1] / Re * detJ * weight;
            Klocal(IV, JW) += 5e-1 * dNdx[ii][2] * dNdx[jj][1] / Re * detJ * weight;
            Klocal(IW, JU) += 5e-1 * dNdx[ii][0] * dNdx[jj][2] / Re * detJ * weight;
            Klocal(IW, JV) += 5e-1 * dNdx[ii][1] * dNdx[jj][2] / Re * detJ * weight;


            // ADVECTION TERM
            for(mm=0;mm<3;mm++)
            {
              Klocal(IU, JU) += 5e-1 * N[ii] * advel[mm] * dNdx[jj][mm] * detJ * weight;
              Klocal(IV, JV) += 5e-1 * N[ii] * advel[mm] * dNdx[jj][mm] * detJ * weight;
              Klocal(IW, JW) += 5e-1 * N[ii] * advel[mm] * dNdx[jj][mm] * detJ * weight;
            }
            
             // PRESSURE TERM
            Klocal(IU, JP) -= N[jj] * dNdx[ii][0] * detJ * weight;
            Klocal(IV, JP) -= N[jj] * dNdx[ii][1] * detJ * weight;
            Klocal(IW, JP) -= N[jj] * dNdx[ii][2] * detJ * weight;
  
            // CONTINUITY TERM
            Klocal(IP, JU) += N[ii] * dNdx[jj][0] * detJ * weight;
            Klocal(IP, JV) += N[ii] * dNdx[jj][1] * detJ * weight;
            Klocal(IP, JW) += N[ii] * dNdx[jj][2] * detJ * weight;
            
           // ** SUPG TERM ** //
           
           // MASS TERM 
           for(mm=0;mm<3;mm++)
           {
             Klocal(IU, JU) += tau * dNdx[ii][mm] * advel[mm] * N[jj] / dt * detJ * weight;
             Klocal(IV, JV) += tau * dNdx[ii][mm] * advel[mm] * N[jj] / dt * detJ * weight;
             Klocal(IW, JW) += tau * dNdx[ii][mm] * advel[mm] * N[jj] / dt * detJ * weight;
           }
            
           // ADVECTION TERM
           for(mm=0;mm<3;mm++)
           {
             for(nn=0;nn<3;nn++){
               Klocal(IU, JU) += 5e-1 * tau * advel[nn] * dNdx[ii][nn] * advel[mm] * dNdx[jj][mm] * detJ * weight;
               Klocal(IV, JV) += 5e-1 * tau * advel[nn] * dNdx[ii][nn] * advel[mm] * dNdx[jj][mm] * detJ * weight;
               Klocal(IW, JW) += 5e-1 * tau * advel[nn] * dNdx[ii][nn] * advel[mm] * dNdx[jj][mm] * detJ * weight;
             }
           }
           
           // PRESSURE TERM
           for(mm=0;mm<3;mm++)
           {
             Klocal(IU, JP) += tau * dNdx[ii][mm] * advel[mm] * dNdx[jj][0] * detJ * weight;
             Klocal(IV, JP) += tau * dNdx[ii][mm] * advel[mm] * dNdx[jj][1] * detJ * weight;
             Klocal(IW, JP) += tau * dNdx[ii][mm] * advel[mm] * dNdx[jj][2] * detJ * weight;
           }

  
          // ** PSPG TERM ** //
          
          // MASS TERM 
           Klocal(IP, JU) += tau * dNdx[ii][0] * N[jj] / dt * detJ * weight;
           Klocal(IP, JV) += tau * dNdx[ii][1] * N[jj] / dt * detJ * weight;
           Klocal(IP, JW) += tau * dNdx[ii][2] * N[jj] / dt * detJ * weight;
           
           // ADVECTION TERM
           for(mm=0;mm<3;mm++)
           {
             Klocal(IP, JU) += 5e-1 * tau * dNdx[ii][0] * advel[mm] * dNdx[jj][mm] * detJ * weight;
             Klocal(IP, JV) += 5e-1 * tau * dNdx[ii][1] * advel[mm] * dNdx[jj][mm] * detJ * weight;
             Klocal(IP, JW) += 5e-1 * tau * dNdx[ii][2] * advel[mm] * dNdx[jj][mm] * detJ * weight;
           }

            // PRESSURE TERM
            Klocal(IP, JP) += tau * K[ii][jj] * detJ * weight;
          }

          // MASS TERM
          Flocal(IU) += N[ii] * vel[0] / dt * detJ * weight;
          Flocal(IV) += N[ii] * vel[1] / dt * detJ * weight;
          Flocal(IW) += N[ii] * vel[2] / dt * detJ * weight;

          // DIFFUSION TERM
          for(mm=0;mm<3;mm++)
          {
            if(mm == 0){s = 2e0; t = 1e0; u = 1e0;}
            if(mm == 1){s = 1e0; t = 2e0; u = 1e0;}
            if(mm == 2){s = 1e0; t = 1e0; u = 2e0;}

            Flocal(IU) -= 5e-1 * s * dNdx[ii][mm]*dvdx[0][mm] / Re * detJ * weight;
            Flocal(IV) -= 5e-1 * t * dNdx[ii][mm]*dvdx[1][mm] / Re * detJ * weight;
            Flocal(IW) -= 5e-1 * u * dNdx[ii][mm]*dvdx[2][mm] / Re * detJ * weight;
          }
          Flocal(IU) -= 5e-1 * dNdx[ii][1] * dvdx[1][0] / Re * detJ * weight;
          Flocal(IU) -= 5e-1 * dNdx[ii][2] * dvdx[2][0] / Re * detJ * weight;
          Flocal(IV) -= 5e-1 * dNdx[ii][0] * dvdx[0][1] / Re * detJ * weight;
          Flocal(IV) -= 5e-1 * dNdx[ii][2] * dvdx[2][1] / Re * detJ * weight;
          Flocal(IW) -= 5e-1 * dNdx[ii][0] * dvdx[0][2] / Re * detJ * weight;
          Flocal(IW) -= 5e-1 * dNdx[ii][1] * dvdx[1][2] / Re * detJ * weight;
          
          // ADVECTION TERM
          for(mm=0;mm<3;mm++)
          {
            Flocal(IU) -= 5e-1 * N[ii] * advel[mm] * dvdx[0][mm] * detJ * weight;
            Flocal(IV) -= 5e-1 * N[ii] * advel[mm] * dvdx[1][mm] * detJ * weight;
            Flocal(IW) -= 5e-1 * N[ii] * advel[mm] * dvdx[2][mm] * detJ * weight;
          }

          // ** SUPG TERM ** //
          
          // MASS TERM
         for(mm=0;mm<3;mm++)
         {
           Flocal(IU) += tau * dNdx[ii][mm] * advel[mm] * vel[0] / dt * detJ * weight;
           Flocal(IV) += tau * dNdx[ii][mm] * advel[mm] * vel[1] / dt * detJ * weight;
           Flocal(IW) += tau * dNdx[ii][mm] * advel[mm] * vel[2] / dt * detJ * weight;
         }
             
         // ADVECTION TERM
         for(mm=0;mm<3;mm++)
         {
           for(nn=0;nn<3;nn++)
           {
             Flocal(IU) -= 5e-1 * tau * vel[nn] * dNdx[ii][nn] * advel[mm] * dvdx[0][mm] * detJ * weight;
             Flocal(IV) -= 5e-1 * tau * vel[nn] * dNdx[ii][nn] * advel[mm] * dvdx[1][mm] * detJ * weight;
             Flocal(IW) -= 5e-1 * tau * vel[nn] * dNdx[ii][nn] * advel[mm] * dvdx[2][mm] * detJ * weight;
           }
         }
              
          // ** PSPG TERM ** //
          
          // MASS TERM 
          Flocal(IP) += tau * dNdx[ii][0] * vel[0] / dt * detJ * weight;
          Flocal(IP) += tau * dNdx[ii][1] * vel[1] / dt * detJ * weight;
          Flocal(IP) += tau * dNdx[ii][2] * vel[2] / dt * detJ * weight;
          
          // ADVECTION TERM
          for(mm=0;mm<3;mm++)
          {
            Flocal(IP) -= 5e-1 * tau * dNdx[ii][0] * advel[mm] * dvdx[0][mm] * detJ * weight;
            Flocal(IP) -= 5e-1 * tau * dNdx[ii][1] * advel[mm] * dvdx[1][mm] * detJ * weight;
            Flocal(IP) -= 5e-1 * tau * dNdx[ii][2] * advel[mm] * dvdx[2][mm] * detJ * weight;
          }
        }
      }
    }
  }
}




void FEM::velocityValue(double (&vel)[3], double (&advel)[3], double (&dvdx)[3][3],VDOUBLE1D &N, VDOUBLE2D &dNdx, const int ic, const int t_itr)
{
  if(t_itr == 0)
  {
    for(int p=0;p<numOfNodeInElm;p++)
    {
      advel[0] = 0;
      advel[1] = 0;
      advel[2] = 0;
    }
    for(int p=0;p<numOfNodeInElm;p++)
    {
      vel[0] = 0e0;
      vel[1] = 0e0;
      vel[2] = 0e0;
    }
    for(int j=0;j<3;j++)
    {
      dvdx[0][j] = 0e0;
      dvdx[1][j] = 0e0;
      dvdx[2][j] = 0e0;
      for(int p=0;p<numOfNodeInElm;p++)
      {
        dvdx[0][j] = 0e0;
        dvdx[1][j] = 0e0;
        dvdx[2][j] = 0e0;
      }
    }
  }
  else if(t_itr == 1)
  {
    for(int p=0;p<numOfNodeInElm;p++)
    {
      advel[0] += N[p] * 1.5 * uf[0][elmFluid[ic]->nodeNumsPrevFluid[p]];
      advel[1] += N[p] * 1.5 * vf[0][elmFluid[ic]->nodeNumsPrevFluid[p]];
      advel[2] += N[p] * 1.5 * wf[0][elmFluid[ic]->nodeNumsPrevFluid[p]];
    }
    for(int p=0;p<numOfNodeInElm;p++)
    {
      vel[0] += N[p] * uf[0][elmFluid[ic]->nodeNumsPrevFluid[p]];
      vel[1] += N[p] * vf[0][elmFluid[ic]->nodeNumsPrevFluid[p]];
      vel[2] += N[p] * wf[0][elmFluid[ic]->nodeNumsPrevFluid[p]];
    }
    for(int j=0;j<3;j++)
    {
      dvdx[0][j] = 0e0;
      dvdx[1][j] = 0e0;
      dvdx[2][j] = 0e0;
      for(int p=0;p<numOfNodeInElm;p++)
      {
        dvdx[0][j] += dNdx[p][j] * uf[0][elmFluid[ic]->nodeNumsPrevFluid[p]];
        dvdx[1][j] += dNdx[p][j] * vf[0][elmFluid[ic]->nodeNumsPrevFluid[p]];
        dvdx[2][j] += dNdx[p][j] * wf[0][elmFluid[ic]->nodeNumsPrevFluid[p]];
      }
    }
  }
  else
  {
    for(int p=0;p<numOfNodeInElm;p++)
    {
      advel[0] += N[p] * (1.5 * uf[0][elmFluid[ic]->nodeNumsPrevFluid[p]] - 0.5 * uf[1][elmFluid[ic]->nodeNumsPrevFluid[p]]);
      advel[1] += N[p] * (1.5 * vf[0][elmFluid[ic]->nodeNumsPrevFluid[p]] - 0.5 * vf[1][elmFluid[ic]->nodeNumsPrevFluid[p]]);
      advel[2] += N[p] * (1.5 * wf[0][elmFluid[ic]->nodeNumsPrevFluid[p]] - 0.5 * wf[1][elmFluid[ic]->nodeNumsPrevFluid[p]]);
    }

    for(int p=0;p<numOfNodeInElm;p++)
    {
      vel[0] += N[p] * uf[0][elmFluid[ic]->nodeNumsPrevFluid[p]];
      vel[1] += N[p] * vf[0][elmFluid[ic]->nodeNumsPrevFluid[p]];
      vel[2] += N[p] * wf[0][elmFluid[ic]->nodeNumsPrevFluid[p]];
    }
    for(int j=0;j<3;j++)
    {
      dvdx[0][j] = 0e0;
      dvdx[1][j] = 0e0;
      dvdx[2][j] = 0e0;
      for(int p=0;p<numOfNodeInElm;p++)
      {
        dvdx[0][j] += dNdx[p][j] * uf[0][elmFluid[ic]->nodeNumsPrevFluid[p]];
        dvdx[1][j] += dNdx[p][j] * vf[0][elmFluid[ic]->nodeNumsPrevFluid[p]];
        dvdx[2][j] += dNdx[p][j] * wf[0][elmFluid[ic]->nodeNumsPrevFluid[p]];
      }
    }
  }
  
}
