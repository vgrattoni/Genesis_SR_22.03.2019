#include "SDDSBeam.h"
#include "MPEProfiling.h"

SDDSBeam::SDDSBeam()
{
  file="";
  ds=0.01;
  output=false; 
  xcen=0;
  ycen=0;
  pxcen=0;
  pycen=0;
  gamma=12000;
  betax=15;
  alphax=0;
  betay=15;
  alphay=0;
  charge=0;
  center=false;
  match=false;
  matchs0=0;
  matchs1=1;
  align=0;
  aligns0=0;
  aligns1=0;
  
}

SDDSBeam::~SDDSBeam(){}

void SDDSBeam::usage(){

  cout << "List of keywords for sddsbeam" << endl;
  cout << "&sddsbeam" << endl;
  cout << " string file = <empty> " << endl;
  cout << " double charge   = 0" << endl;
  cout << " double slicewidth = 0.01" << endl;
  cout << " bool output = false " << endl;
  cout << " bool center = false " << endl;
  cout << " double gamma0 = gammaref " << endl;
  cout << " double x0 = 0 " << endl;
  cout << " double y0 = 0 " << endl;
  cout << " double px0 = 0 " << endl;
  cout << " double py0 = 0 " << endl;
  cout << " bool match = false " << endl;
  cout << " double betax  = 15 / matched" << endl;
  cout << " double alphax  = 0 / matched" << endl;
  cout << " double betay  = 15 / matched" << endl;
  cout << " double alphay  = 0 / matched" << endl;
  cout << " double match_start = 0 " << endl;
  cout << " double match_end = 1 " << endl;
  cout << " int align = 0 " << endl;
  cout << " double align_start = 0 " << endl;
  cout << " double align_end = 1 " << endl;
  cout << "&end" << endl << endl;
  return;
}

bool SDDSBeam::init(int inrank, int insize, map<string,string> *arg, Beam *beam, Setup *setup, Time *time, Lattice *lat)
{

  rank=inrank;
  size=insize;

  gamma=setup->getReferenceEnergy();           // get default energy from setup input deck
  lat->getMatchedOptics(&betax,&alphax,&betay,&alphay);  // use matched value if calculated

  double lambda=setup->getReferenceLength();   // reference length for theta
  double sample=static_cast<double>(time->getSampleRate());         // check slice length

  bool one4one=setup->getOne4One();
  bool shotnoise=setup->getShotNoise();
  int npart=setup->getNpart();
  int nbins=setup->getNbins();

  double theta0=4.*asin(1.);
  if (one4one) {
      nbins=1;
      theta0*=sample;
  }
  if ( (npart % nbins) != 0){
    if (rank==0) { cout << "*** Error: NPART is not a multiple of NBINS" << endl; } 
    return false;
  }
  
  theta0/=static_cast<double>(nbins);
 
  map<string,string>::iterator end=arg->end();

  if (arg->find("file")!=end)       {file   = arg->at("file"); arg->erase(arg->find("file"));}
  if (arg->find("charge")!=end)     {charge = atof(arg->at("charge").c_str());     arg->erase(arg->find("charge"));}
  if (arg->find("slicewidth")!=end) {ds     = atof(arg->at("slicewidth").c_str()); arg->erase(arg->find("slicewidth"));}
  if (arg->find("match_start")!=end)    {matchs0    = atof(arg->at("match_start").c_str());    arg->erase(arg->find("match_start"));}
  if (arg->find("match_end")!=end)      {matchs1    = atof(arg->at("match_end").c_str());      arg->erase(arg->find("match_end"));}
  if (arg->find("align_start")!=end)    {aligns0    = atof(arg->at("align_start").c_str());    arg->erase(arg->find("align_start"));}
  if (arg->find("align_end")!=end)      {aligns1    = atof(arg->at("align_end").c_str());      arg->erase(arg->find("align_end"));}
  if (arg->find("betax")!=end)    {betax = atof(arg->at("betax").c_str()); arg->erase(arg->find("betax"));}
  if (arg->find("betay")!=end)    {betay = atof(arg->at("betay").c_str()); arg->erase(arg->find("betay"));}
  if (arg->find("alphax")!=end)   {alphax= atof(arg->at("alphax").c_str());arg->erase(arg->find("alphax"));}
  if (arg->find("alphay")!=end)   {alphay= atof(arg->at("alphay").c_str());arg->erase(arg->find("alphay"));}
  if (arg->find("x0")!=end)       {xcen  = atof(arg->at("x0").c_str());    arg->erase(arg->find("x0"));}
  if (arg->find("y0")!=end)       {ycen  = atof(arg->at("y0").c_str());    arg->erase(arg->find("y0"));}
  if (arg->find("px0")!=end)      {pxcen = atof(arg->at("px0").c_str());   arg->erase(arg->find("px0"));}
  if (arg->find("py0")!=end)      {pycen = atof(arg->at("py0").c_str());   arg->erase(arg->find("py0"));}
  if (arg->find("gamma0")!=end)   {gamma = atof(arg->at("gamma0").c_str());arg->erase(arg->find("gamma0"));}
  if (arg->find("align")!=end)    {align = atoi(arg->at("align").c_str()); arg->erase(arg->find("align"));}
  if (arg->find("match")!=end)    {match = atob(arg->at("match").c_str()); arg->erase(arg->find("match"));}
  if (arg->find("center")!=end)   {center= atob(arg->at("center").c_str());arg->erase(arg->find("center"));}
  if (arg->find("output")!=end)   {output= atob(arg->at("output").c_str());arg->erase(arg->find("output"));}



  if (arg->size()!=0){
    if (rank==0){ cout << "*** Error: Unknown elements in &sddsbeam" << endl; this->usage();}
    return false;
  }

  mpe.logCalc(false,true,"System call SDDS2HDF5");
  if (rank==0) { cout << "Converting SDDS distribution file " << file << " into HDF5 file... " << endl;}
  string command="sdds2hdf-dist.sh " + file;
  int status=0;
  if (rank==0){
    status=system(command.c_str());
    if (status!=0){ cout << "*** Error: SDDS Distribution file " << file << " does not exist" << endl;} 
  }

  mpe.logCalc(true,true,"System call SDDS2HDF5");


  MPI::COMM_WORLD.Bcast(&status,1,MPI::INT,0); // this statement keeps also all nodes on hold till the converison has been done
  if (status!=0) {return false;}



  if (rank==0) { cout << "Importing converted distribution file... " << endl;}


  mpe.logIO(false,true,"Read External Distribution"); 
  string anafile=file;
  string h5file=file.append(".h5");
  

  hid_t pid = H5Pcreate(H5P_FILE_ACCESS);
  H5Pset_fapl_mpio(pid,MPI_COMM_WORLD,MPI_INFO_NULL);
  hid_t fid=H5Fopen(file.c_str(),H5F_ACC_RDONLY,pid);
  H5Pclose(pid);

  string dset="t";
  int ntotal=getDatasetSize(fid, (char *)dset.c_str());
  double dQ=charge/static_cast<double> (ntotal);

  int nchunk=ntotal/size;
  if ((ntotal % size) !=0) {nchunk++;}

  int nsize=nchunk;
  if ((rank*nchunk+nsize)>ntotal) { nsize=ntotal-rank*nchunk; }

  t.resize(nsize);
  g.resize(nsize);
  x.resize(nsize);
  y.resize(nsize);
  px.resize(nsize);
  py.resize(nsize);

  string dname="t";
  readDouble1D(fid,dname.c_str(),&t[0],nsize,rank*nchunk);
  dname="p"; 
  readDouble1D(fid,dname.c_str(),&g[0],nsize,rank*nchunk);
  dname="x"; 
  readDouble1D(fid,dname.c_str(),&x[0],nsize,rank*nchunk);
  dname="xp"; 
  readDouble1D(fid,dname.c_str(),&px[0],nsize,rank*nchunk);
  dname="y"; 
  readDouble1D(fid,dname.c_str(),&y[0],nsize,rank*nchunk);
  dname="yp"; 
  readDouble1D(fid,dname.c_str(),&py[0],nsize,rank*nchunk);

  H5Fclose(fid);

  mpe.logIO(true,true,"Read External Distribution"); 


  if (rank==0) { cout << "Analysing external distribution... " << endl;}


  for (int i=0; i<nsize; i++){
    t[i]*=-3e8;
    g[i]+=1.;
  }


  double tmin,tmax;

  double tmp=*min_element(t.begin(),t.end());
  MPI::COMM_WORLD.Allreduce(&tmp,&tmin,1,MPI::DOUBLE,MPI::MIN);
  tmp=*max_element(t.begin(),t.end());
  MPI::COMM_WORLD.Allreduce(&tmp,&tmax,1,MPI::DOUBLE,MPI::MAX);
  tmp=accumulate(g.begin(),g.end(),0);


  double ttotal=tmax-tmin;

  for (int i=0; i<nsize; i++){
    t[i]-=tmin;
  }


  if (rank==0) {
    cout << "Analysis of the imported distribution" << endl;  
    cout << "   Total Bunch Length  (microns): " << ttotal*1e6 << endl;
  }

  this->analyse(ttotal,nsize);

  if (center) {
    if (rank==0){cout << "Centering external distribution..." << endl; }
    double ratio=sqrt(gavg/gamma);
    gavg=gamma-gavg;
    xavg=xcen-xavg;
    yavg=ycen-yavg;
    pxavg=pxcen-pxavg;
    pyavg=pycen-pyavg;
    for (int i=0; i<nsize; i++){
      g[i]+=gavg;
      x[i]+=xavg;
      y[i]+=yavg;
      px[i]+=pxavg;
      py[i]+=pyavg;
      x[i]*=ratio;   // rescaling is needed to preserve emittance
      y[i]*=ratio;
      px[i]*=ratio;
      py[i]*=ratio;
    }
  }

  if (match) {
    if (rank==0){cout << "Matching external distribution..." << endl; }
    for (int i=0; i<nsize; i++){
      px[i]+=(ax/bx)*x[i];
      py[i]+=(ay/by)*y[i];
      x[i]*=sqrt(betax/bx);
      y[i]*=sqrt(betay/by);
      px[i]*=sqrt(bx/betax);
      py[i]*=sqrt(by/betay);
      px[i]-=(alphax/betax)*x[i];
      py[i]-=(alphay/betay)*y[i];
    }
  } 

  if ((match)||(center)){
    if (rank==0) {cout << "Reanalysing matched and aligned distribution..." << endl; }
    this->analyse(ttotal,nsize);
  }


  // slicing external distribution to fill given slizes

  vector<double> s;
  int nslice=time->getPosition(&s);
  int node_off=time->getNodeOffset();
  int node_len=time->getNodeNSlice();
  beam->beam.resize(node_len);

  double smin=s[node_off];
  double smax=s[node_off+node_len-1];
  
  if (rank==0) {cout << "Sorting external distribution..." << endl; }

  double dslen=ds*ttotal;  // ds is the relative width to extract the samples (equivalent to 1/NDCUT)

  vector<vector<Particle> > dist;
  dist.resize(1);

  // copying all particles into the dist vector to enable sorting
  Particle part;
  for (int i=0; i<nsize; i++){
      part.theta=t[i];
      part.gamma=g[i];
      part.x=x[i];
      part.y=y[i];
      part.px=px[i]*g[i];
      part.py=py[i]*g[i];
      dist[0].push_back(part);
  }
  t.clear();
  g.clear();
  x.clear();
  y.clear();
  px.clear();
  py.clear();


  Sorting sort;
  sort.init(rank,size,smin,smax,dslen,6,false,true); 
  sort.globalSort(&dist,0);  

  // now each node has all the particles, which is needed for the phase space reconstruction
  if (rank==0) {cout << "Generarting internal particle distribution..." << endl; }
  mpe.logLoading(false,"External Distribution");

  beam->current.resize(node_len);
  beam->beam.resize(node_len);

  this->initRandomSeq(setup->getSeed());
  ShotNoise sn;
  sn.init(setup->getSeed(),rank);

  int nwork=100;
  Particle *work;
  work=new Particle [nwork];
  for (int islice=0; islice<node_len;islice++){

    // step 1 - select all particles needed for the reconstruction
    double sloc=s[islice+node_off];
    for (int i=0; i<dist[0].size();i++){
      if ((dist[0].at(i).theta>(sloc-0.5*dslen))&&(dist[0].at(i).theta<(sloc+0.5*dslen))){
	beam->beam.at(islice).push_back(dist[0].at(i));
      }
    }
   
    // step 2 - calculate the current and number of particles.
    int ncount = beam->beam.at(islice).size();
    int mpart;
    beam->current[islice]=static_cast<double>(ncount)*dQ*3e8/dslen;
    if (one4one){
      npart=static_cast<int>(round(beam->current[islice]*lambda*sample/ce));
      mpart=npart;
    } else {
      mpart=npart/nbins;
    }

    // step 3 - bring initial distribution to the right size

    if (beam->beam.at(islice).size() >= mpart){
      this->removeParticles(&beam->beam.at(islice),mpart);
    } else {
      this->addParticles(&beam->beam.at(islice),mpart);
    }

    // step 4 - refill particle phase completely new
    for (int i=0;i<beam->beam.at(islice).size();i++){
      beam->beam.at(islice).at(i).theta=theta0*ran->getElement();
    }

    if (!one4one){
      mpart=beam->beam.at(islice).size();
      beam->beam.at(islice).resize(mpart*nbins);
      for (int i=mpart; i>0; i--){
        int i1=i-1;
        int i2=nbins*i1;
        for (int j=0;j<nbins;j++){
          beam->beam.at(islice).at(i2+j).gamma=beam->beam.at(islice).at(i1).gamma;
  	  beam->beam.at(islice).at(i2+j).x    =beam->beam.at(islice).at(i1).x;
          beam->beam.at(islice).at(i2+j).y    =beam->beam.at(islice).at(i1).y;
          beam->beam.at(islice).at(i2+j).px   =beam->beam.at(islice).at(i1).px;
          beam->beam.at(islice).at(i2+j).py   =beam->beam.at(islice).at(i1).py;
          beam->beam.at(islice).at(i2+j).theta=beam->beam.at(islice).at(i1).theta+j*theta0;     
	}
      }
      double ne=round(beam->current[islice]*lambda*sample/ce);
      if (mpart*nbins>nwork){
      	nwork=mpart*nbins;
        delete[] work;
        work=new Particle [nwork];
      }
      for (int i=0;i<mpart;i++){
      	work[i].theta=beam->beam.at(islice).at(i).theta;  
      }
      sn.applyShotNoise(work,mpart*nbins,nbins,ne); /////////////////////////////////////////////////////////////////////// shouldn-t it be copy back
      for (int i=0;i<mpart;i++){
      	beam->beam.at(islice).at(i).theta=work[i].theta;  
      }

    }
   
  }

  mpe.logLoading(true,"External Distribution");

  dist[0].clear();
  delete ran;
  delete [] work;
  
  if (output){
    mpe.logIO(false,true,"Write Distribution Analysis"); 
    if (rank==0) {cout << "Writing analysis to file..." << endl; }

    Output *out=new Output;
    string outfile=anafile.append(".ana.h5");

    hid_t pid = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_fapl_mpio(pid,MPI_COMM_WORLD,MPI_INFO_NULL);
    hid_t fid=H5Fcreate(outfile.c_str(),H5F_ACC_TRUNC, H5P_DEFAULT,pid); 
    H5Pclose(pid);
    H5Fclose(fid);

    out->open(outfile.c_str(),size,NULL,nslice,1,node_off,node_len);
    out->writeBeam(0,beam);
    out->writeGlobalBeam(beam,bx,by,ax,ay);
    out->close();
    delete out;
    mpe.logIO(true,true,"Write Distribution Analysis"); 

  }


  mpe.logEvent("End: SDDSBeam::init");


  return true;


}


void SDDSBeam::addParticles(vector<Particle> *beam, int mpart){
   
  int  ndist=beam->size();
  if (ndist==0){
    return;
  }

  // step 1 - calculate the center and the rms beam size
  double g1,x1,y1,px1,py1,g2,x2,y2,px2,py2;
  g1=0;
  x1=0;
  y1=0;
  px1=0;
  py1=0;
  g2=0;
  x2=0;
  y2=0;
  px2=0;
  py2=0;
  for (int i=0; i<ndist; i++){
    g1 +=beam->at(i).gamma;
    g2 +=beam->at(i).gamma * beam->at(i).gamma;
    x1 +=beam->at(i).x;
    x2 +=beam->at(i).x * beam->at(i).x;
    px1+=beam->at(i).px;
    px2+=beam->at(i).px * beam->at(i).px;
    y1 +=beam->at(i).y;
    y2 +=beam->at(i).y * beam->at(i).y;
    py1+=beam->at(i).py;
    py2+=beam->at(i).py * beam->at(i).py;
  }
  double scl=1/static_cast<double>(ndist);

  g1*=scl;
  g2=sqrt(fabs(g2*scl-g1*g1));
  x1*=scl;
  x2=sqrt(fabs(x2*scl-x1*x1));
  px1*=scl;
  px2=sqrt(fabs(px2*scl-px1*px1));
  y1*=scl;
  y2=sqrt(fabs(y2*scl-y1*y1));
  py1*=scl;
  py2=sqrt(fabs(py2*scl-py1*py1));

  // step 2 - invert the beam size for normalization and check for "cold" dimensions, e.g. zero energy spread
  if (g2==0) { g2=1; } else { g2=1/g2; }
  if (x2==0) { x2=1; } else { x2=1/x2; }
  if (y2==0) { y2=1; } else { y2=1/y2; }
  if (px2==0) { px2=1; } else { px2=1/px2; }
  if (py2==0) { py2=1; } else { py2=1/py2; }

  // step 3 - normalize distribution so that it is aligned to the origin and has an rms size of unity in all dimensions
  for (int i=0; i<ndist; i++){
    beam->at(i).gamma=(beam->at(i).gamma - g1)*g2;
    beam->at(i).x    =(beam->at(i).x     - x1)*x2;
    beam->at(i).y    =(beam->at(i).y     - y1)*y2;
    beam->at(i).px   =(beam->at(i).px    - px1)*px2;
    beam->at(i).py   =(beam->at(i).py    - py1)*py2;
  }
  
  // step 4 - add particles
  int ndist0=ndist;
  Particle par;
  while (ndist<mpart){
    int n1=static_cast<int>(floor(static_cast<double>(ndist0)*ran->getElement()));
    double rmin=1e9;
    int n2=n1;
    for (int i=0; i<ndist0;i++){
       double r=this->distance(beam->at(n1),beam->at(i)); 
       if ((r<rmin) && ( i!=n1 )) {
           n2=i;
           rmin=r;
       }
    }
    par.gamma=0.5*(beam->at(n1).gamma+beam->at(n2).gamma)+(2*ran->getElement()-1)*(beam->at(n1).gamma-beam->at(n2).gamma);
    par.x =0.5*(beam->at(n1).x +beam->at(n2).x) +(2*ran->getElement()-1)*(beam->at(n1).x -beam->at(n2).x);
    par.px=0.5*(beam->at(n1).px+beam->at(n2).px)+(2*ran->getElement()-1)*(beam->at(n1).px-beam->at(n2).px);
    par.y =0.5*(beam->at(n1).y +beam->at(n2).y) +(2*ran->getElement()-1)*(beam->at(n1).y -beam->at(n2).y);
    par.py=0.5*(beam->at(n1).py+beam->at(n2).py)+(2*ran->getElement()-1)*(beam->at(n1).py-beam->at(n2).py);
    beam->push_back(par);    
    ndist++;
  }
 

  // step 5 - scale back

  for (int i=0; i<beam->size(); i++){
    beam->at(i).gamma=beam->at(i).gamma/g2 + g1;
    beam->at(i).x    =beam->at(i).x/x2 + x1;
    beam->at(i).y    =beam->at(i).y/y2 + y1;
    beam->at(i).px   =beam->at(i).px/px2 + px1; 
    beam->at(i).py   =beam->at(i).py/py2 + py1;
  }


  return;
}


double  SDDSBeam::distance(Particle p1, Particle p2){

  double tmp=p1.gamma-p2.gamma;  
  double r=tmp*tmp*ran->getElement();
  tmp=p1.x-p2.x;  
  r+=tmp*tmp*ran->getElement();
  tmp=p1.y-p2.y;  
  r+=tmp*tmp*ran->getElement();
  tmp=p1.px-p2.px;  
  r+=tmp*tmp*ran->getElement();
  tmp=p1.py-p2.py;  
  r+=tmp*tmp*ran->getElement();
  return r;
}


void SDDSBeam::removeParticles(vector<Particle> *beam,int mpart)
{
  int ndist=beam->size();
  while(ndist>mpart){
    int idx=static_cast<int>(floor(static_cast<double>(ndist)*ran->getElement()));
    beam->at(idx)=beam->at(ndist-1);
    ndist--;
  }
  beam->resize(mpart);
  return;
}

void SDDSBeam::analyse(double ttotal,int nsize)
{

  int ncount = 0;
  int nmean;
  double mt0=matchs0*ttotal;
  double mt1=matchs1*ttotal;

  double a2,a1,b2,b1,ab; 
  double c2,c1,d2,d1,cd; 
  double e1=0;
  a2=0;a1=0;b2=0;b1=0;ab=0;
  c2=0;c1=0;d2=0;d1=0;cd=0;
  
  //  cout << "Rank: " << rank << " Size: " << nsize << endl;

  for (int i=0; i <nsize;i++){
    if ((t[i]>mt0)&&(t[i]<mt1)){
      ncount++;
      a1+=x[i];
      a2+=x[i]*x[i];
      b1+=px[i];
      b2+=px[i]*px[i];
      ab+=x[i]*px[i];
      c1+=y[i];
      c2+=y[i]*y[i];
      d1+=py[i];
      d2+=py[i]*py[i];
      cd+=y[i]*py[i];
      e1+=g[i];
    }
  }


  MPI::COMM_WORLD.Allreduce(&ncount,&nmean,1,MPI::INT,MPI::SUM);
  MPI::COMM_WORLD.Allreduce(&e1,&gavg, 1,MPI::DOUBLE,MPI::SUM);
  MPI::COMM_WORLD.Allreduce(&a1,&xavg, 1,MPI::DOUBLE,MPI::SUM);
  MPI::COMM_WORLD.Allreduce(&b1,&pxavg,1,MPI::DOUBLE,MPI::SUM);
  MPI::COMM_WORLD.Allreduce(&c1,&yavg, 1,MPI::DOUBLE,MPI::SUM);
  MPI::COMM_WORLD.Allreduce(&d1,&pyavg,1,MPI::DOUBLE,MPI::SUM);
  MPI::COMM_WORLD.Allreduce(&a2,&xvar, 1,MPI::DOUBLE,MPI::SUM);
  MPI::COMM_WORLD.Allreduce(&b2,&pxvar,1,MPI::DOUBLE,MPI::SUM);
  MPI::COMM_WORLD.Allreduce(&ab,&xpx,  1,MPI::DOUBLE,MPI::SUM);
  MPI::COMM_WORLD.Allreduce(&c2,&yvar, 1,MPI::DOUBLE,MPI::SUM);
  MPI::COMM_WORLD.Allreduce(&d2,&pyvar,1,MPI::DOUBLE,MPI::SUM);
  MPI::COMM_WORLD.Allreduce(&cd,&ypy,  1,MPI::DOUBLE,MPI::SUM);

  if (nmean>0){
    double tmp=1./static_cast<double>(nmean);
    gavg*=tmp;
    xavg*=tmp;
    yavg*=tmp;
    pxavg*=tmp;
    pyavg*=tmp;
    xvar*=tmp;
    yvar*=tmp;
    pxvar*=tmp;
    pyvar*=tmp;
    xpx*=tmp;
    ypy*=tmp;
  }

  ex=sqrt(fabs((xvar-xavg*xavg)*(pxvar-pxavg*pxavg)-(xpx-xavg*pxavg)*(xpx-xavg*pxavg)))*gavg;
  ey=sqrt(fabs((yvar-yavg*yavg)*(pyvar-pyavg*pyavg)-(ypy-yavg*pyavg)*(ypy-yavg*pyavg)))*gavg;
  bx=(xvar-xavg*xavg)/ex*gavg;
  by=(yvar-yavg*yavg)/ey*gavg;
  ax=-(xpx-xavg*pxavg)*gavg/ex;
  ay=-(ypy-yavg*pyavg)*gavg/ey;

  if (rank==0) {
       cout << "   Length for Matching (microns): " << (mt1-mt0)*1e6 << endl; 
       cout << "   Energy                  (MeV): " << gavg*eev*1e-6 << endl;
       cout << "   Norm. Emittance in x (micron): " << ex*1e6 << endl;
       cout << "   Norm. Emittance in y (micron): " << ey*1e6 << endl;
       cout << "   Beta Function in x        (m): " << bx << endl;
       cout << "   Beta Function in y        (m): " << by << endl;
       cout << "   Alpha Function in x          : " << ax << endl;
       cout << "   Alpha Function in y          : " << ay << endl;
       cout << "   Beam center in x     (micron): " << xavg*1e6 << endl;
       cout << "   Beam center in y     (micron): " << yavg*1e6 << endl;
       cout << "   Beam center in px            : " << pxavg << endl;
       cout << "   Beam center in py            : " << pyavg << endl;
  }
  return;


}

void SDDSBeam::initRandomSeq(int base)
{
     RandomU rseed(base);
     double val;
     for (int i=0; i<=rank+10000;i++){
        val=rseed.getElement();
     }
     val*=1e9;
     int locseed=static_cast<int> (round(val));
     ran  =  new RandomU (locseed);
     return;
}
