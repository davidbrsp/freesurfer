#include "mris_topology.h"
#include "topology/patchdisk.h"

#define __PRINT_MODE 0
#define WHICH_OUTPUT stderr

bool doesMRISselfIntersect(MRIS *mris_work,TOPOFIX_PARMS &parms);

//check the new vertices : val2, val2bak... marked2=-1 ?

extern "C" bool MRIScorrectDefect(MRIS *mris, int defect_number,TOPOFIX_PARMS &parms){

  int euler = MRISgetEuler(mris, defect_number);
	fprintf(WHICH_OUTPUT,"\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\nCorrecting Topology of defect %d with euler number %d\n",parms.defect_number,euler);

  if(euler == 1) {
    fprintf(WHICH_OUTPUT,"   Nothing to correct for defect %d!!\n",parms.defect_number);
    return true;
  }

	MRISinitDefectParameters(mris,&parms);

#if __PRINT_MODE
	fprintf(WHICH_OUTPUT,"   extracting MRIP\n");
#endif
	MRIP *mrip = MRIPextractFromMRIS(mris,defect_number);
	if(mrip==NULL) {
		 TOPOFIXfreeDP(&parms);
		 return false;
	}

#if __PRINT_MODE 
	euler = MRISgetEuler(mrip->mris);
	fprintf(WHICH_OUTPUT,"BEFORE TOP euler is %d\n",euler);
#endif

	bool correct = MRIScorrectPatchTopology(mrip->mris,parms);
	if(correct == false){
		fprintf(WHICH_OUTPUT,"PBM : Could not correct topology\n");
		MRIPfree(&mrip);
		TOPOFIXfreeDP(&parms);
		return false;
	}

#if __PRINT_MODE
	fprintf(WHICH_OUTPUT,"transferring corrections\n");
#endif


	MRISaddMRIP(mris,mrip);


#if __PRINT_MODE
  euler = MRISgetEuler(mris);
 fprintf(WHICH_OUTPUT,  "Now, euler is %d\n",euler);
#endif

	MRIPfree(&mrip);
	TOPOFIXfreeDP(&parms);

	//	fprintf(WHICH_OUTPUT,"\n");
	//printing out results
	if(parms.verbose){
		fprintf(WHICH_OUTPUT,"INITIAL FITNESS :  %3.5f \n",parms.initial_fitness);
		fprintf(WHICH_OUTPUT,"FINAL FITNESS   :  %3.5f \n",parms.fitness);
		fprintf(WHICH_OUTPUT,"# generated patches : %d (%d self-intersecting)\n",parms.ngeneratedpatches,parms.nselfintersectingpatches);
	}

	return true; 
}

bool MRIScorrectPatchTopology(MRIS* &mris,TOPOFIX_PARMS &parms){
#if __PRINT_MODE
	fprintf(WHICH_OUTPUT,"Correcting Topology\n");
#endif

  //first initialize the right parameters for this defect
  //what is the maximum face generating a cutting loop
  parms.max_face = mris->nfaces;
	parms.nattempts = nint(parms.nattempts_percent*mris->nfaces+1);
  parms.nminimal_attempts=nint(parms.minimal_loop_percent*mris->nfaces+parms.nminattempts);
	
	MRIScomputeNormals(mris);
  MRIScomputeTriangleProperties(mris);
  MRISsaveVertexPositions(mris,ORIGINAL_VERTICES);
	MRISinitDefectPatch(mris,&parms);
	parms.initial_fitness = MRIScomputeFitness(mris,&parms);
	parms.fitness = -1.0;
	parms.ngeneratedpatches=0;
	parms.nselfintersectingpatches=0;
	int nloops = (1-MRISgetEuler(mris))/2;
	//we limit the total number of attempts to 2000!
	parms.nattempts = __MIN(2000/nloops,parms.nattempts);
	//parms.nminimal_attempts = __MIN(__MAX(1000/nloops,500),parms.nminimal_attempts);

	
		for(int n = 0 ; n < nloops ; n++){
		if(parms.verbose>=VERBOSE_MODE_MEDIUM){
			fprintf(WHICH_OUTPUT,"      max face = %d(%d) - loop = %d (%d)  - ntries = [%d,%d]\n",
							parms.max_face,mris->nfaces,n+1,nloops, parms.nattempts,parms.nminimal_attempts);
		}
		bool correct = MRISincreaseEuler(mris,parms);
		if(correct == false) return false;
	}
	return true;
}


extern "C" bool MRISincreaseEuler(MRIS* &mris,TOPOFIX_PARMS &parms){
  int nattempts=parms.nattempts;

	static int s_nbr = 0;

	char fname[STRLEN] ;

	int nintersections=0,npatches=0;

	MRIS *best_mris = NULL;
	double best_fitness = -1;

#if __PRINT_MODE
  fprintf(WHICH_OUTPUT,"increasing euler number\n");
#endif
	
  for(int n = 0 ; n < nattempts ; n++){
		if(parms.verbose>=VERBOSE_MODE_HIGH)
			fprintf(stderr, "\r      %3.2f%% [%5d,%5d]",n*100.0/nattempts,n,nattempts);

    //generate a surface
    Surface *surface = MRIStoSurface(mris);
    surface->disk=(PatchDisk*)parms.patchdisk;
 
		//compute the euler number of the surface (mandatory for CutPatch!)
		int euler = surface->GetEuler();
		int init_euler = euler;

#if __PRINT_MODE
		fprintf(WHICH_OUTPUT,"BEFORE %d (%d: %d, %d)\n",euler,n,surface->nvertices,surface->nfaces);
#endif

		//increase the euler number by 2
    int correct;
		if(n == 0 ) 
			correct = surface->CutPatch(-2,parms.max_face,parms.nminimal_attempts);
		else			
			correct = surface->CutPatch(-1,parms.max_face,10);//always trying 10 times at least

		npatches++;

		if(correct < 0){ 
			fprintf(WHICH_OUTPUT,"\r      PBM: Euler Number incorrect for surface (defect %d)\n",parms.defect_number);
			delete surface;
			continue;
		}
#if __PRINT_MODE
		fprintf(WHICH_OUTPUT,"AFTER %d (%d,%d)\n",surface->GetEuler(),surface->nvertices,surface->nfaces);
#endif

		//transfer data into MRIS structure
    MRIS *mris_work = SurfaceToMRIS(surface,NULL);
		delete surface;
		MRIScopyHeader(mris,mris_work);
		MRISsaveVertexPositions(mris_work,INFLATED_VERTICES); //saving current vertex positions into inflated
		
		euler = MRISgetEuler(mris_work);
		if(euler != init_euler+2){
			fprintf(WHICH_OUTPUT,"\r      PBM: Euler Number incorrect for mris (defect %d)\n",parms.defect_number);
			MRISfree(&mris_work);
			continue;
		}

#if __PRINT_MODE
    fprintf(WHICH_OUTPUT,"Topology -> %d \n",euler);
#endif

		// we have a correct surface : evaluate if valid
		MRISinitDefectPatch(mris_work,&parms);
		
		if(parms.mode == 1){ //very fast mode 
			MRISsaveVertexPositions(mris_work,ORIGINAL_VERTICES);
			//MRISmarkPatchVertices(mris_work,&parms,0);
			MRISmarkPatchVertices(mris_work,&parms,mris->nvertices);
			for(int n = 0 ; n < 4 ; n++) MRISexpandMarked(mris_work);
			MRISmarkBorderVertices(mris_work,&parms,0);

			double fitness;
			if(parms.smooth || parms.match){
				MRISdefectMatch(mris_work,&parms);
				npatches++;
				fitness = MRIScomputeFitness(mris_work,&parms);
				//if(parms.verbose>=VERBOSE_MODE_MEDIUM) fprintf(WHICH_OUTPUT,"\r      fitness (M)is %3.5f \n",fitness);
			}else
				fitness = MRIScomputeFitness(mris_work,&parms);

			//update if necessary
			if(best_mris == NULL || fitness > best_fitness){
				//check if self-intersect
				bool selfintersect = doesMRISselfIntersect(mris_work,parms);
				if(selfintersect){
					nintersections++;
					if(parms.verbose>=VERBOSE_MODE_HIGH) fprintf(WHICH_OUTPUT,"\r      SELF-INTERSECTING PATCH\n");
					//try without matching 
					//MRISrestoreVertexPositions(mris_work,INFLATED_VERTICES);
					//MRISsaveVertexPositions(mris_work,ORIGINAL_VERTICES);
					//fitness = MRIScomputeFitness(mris_work,&parms);
					//if(fitness > best_fitness){
					//	selfintersect = doesMRISselfIntersect(mris_work,parms);
					MRISfree(&mris_work);
					continue;
				}else{
					if(best_mris) MRISfree(&best_mris);
					best_mris = mris_work;
					best_fitness = fitness;
					if(parms.verbose>=VERBOSE_MODE_MEDIUM){
						fprintf(WHICH_OUTPUT,"\r      BEST FITNESS (M) is %3.5f \n",best_fitness);
						MRISprintInfo(&parms);
						if(parms.write){
							sprintf(fname,"./%s_%d_%d.asc" , parms.fname , parms.defect_number,s_nbr++);
							MRISwrite(mris_work,fname);
						}
					}
				}
			}else{
				MRISfree(&mris_work);
			}
			continue;
		}

		//compute associated fitness
		double fitness = MRIScomputeFitness(mris_work,&parms);
		//if(parms.verbose>=VERBOSE_MODE_MEDIUM) fprintf(WHICH_OUTPUT,"\r      fitness (o)is %3.5f \n",fitness);
		
		//update if necessary
    if(best_mris == NULL || fitness > best_fitness){
			// check if self-intersect
			bool selfintersect = doesMRISselfIntersect(mris_work,parms);
			if(selfintersect){
				nintersections++;
				if(parms.verbose>=VERBOSE_MODE_HIGH) fprintf(WHICH_OUTPUT,"\r      SELF-INTERSECTING PATCH\n");
				if(parms.minimal_mode && best_mris==NULL){
					nattempts = __MAX(50,nattempts);
				}
				//MRISfree(&mris_work);
				//continue;
				//check if the active contour deformation will help 
			}else{
				if(best_mris) MRISfree(&best_mris);
				best_mris = MRISduplicateOver(mris_work,1);
				best_fitness = fitness;
				if(parms.verbose>=VERBOSE_MODE_MEDIUM){
					fprintf(WHICH_OUTPUT,"\r      BEST FITNESS (o)is %3.5f \n",best_fitness);
					MRISprintInfo(&parms);
					if(parms.write){
						sprintf(fname,"./%s_%d_%d.asc",parms.fname,parms.defect_number,s_nbr++);
						MRISwrite(mris_work,fname);
					}
				}
			}
    };

		//now try to use local intensities to improve fitness
		//first mark vertices
		MRISmarkPatchVertices(mris_work,&parms,mris->nvertices);
		for(int n = 0 ; n < 4 ; n++) MRISexpandMarked(mris_work);
		MRISmarkBorderVertices(mris_work,&parms,0);
		
		if(parms.smooth || parms.match){
			MRISdefectMatch(mris_work,&parms);
			npatches++;
			fitness = MRIScomputeFitness(mris_work,&parms);
			//if(parms.verbose>=VERBOSE_MODE_MEDIUM) fprintf(WHICH_OUTPUT,"\r      fitness (m)is %3.5f \n",fitness);
		}
		
		//update if necessary 
		if(best_mris == NULL || fitness > best_fitness){
			//check if self-intersect
			bool selfintersect = doesMRISselfIntersect(mris_work,parms);
      if(selfintersect){
				nintersections++;
				if(parms.verbose>=VERBOSE_MODE_HIGH) fprintf(WHICH_OUTPUT,"\r      SELF-INTERSECTING PATCH\n");
        MRISfree(&mris_work);
        continue;
      }else{
				if(best_mris) MRISfree(&best_mris);
				best_mris = mris_work;
				best_fitness = fitness;
				if(parms.verbose>=VERBOSE_MODE_MEDIUM){
					fprintf(WHICH_OUTPUT,"\r      BEST FITNESS (M) is %3.5f \n",best_fitness);
					MRISprintInfo(&parms);
					if(parms.write){
						sprintf(fname,"./%s_%d_%d.asc" , parms.fname , parms.defect_number,s_nbr++);
						MRISwrite(mris_work,fname);
					}
				}
			}
		}else{
			MRISfree(&mris_work);
		}
	}
	if(parms.verbose>=VERBOSE_MODE_HIGH)
		fprintf(WHICH_OUTPUT,"\r                             \r");

	if(parms.verbose>=VERBOSE_MODE_MEDIUM)
		fprintf(WHICH_OUTPUT,"      %d patches have been generated - %d self-intersected\n",npatches,nintersections);

	//update the surface
	if(best_mris == NULL ){
		fprintf(WHICH_OUTPUT,"PBM : Could Not Increase Euler Number for defect %d\n",parms.defect_number);
		return false;
	}
	
	MRISfree(&mris);
	mris = best_mris;
	parms.fitness = best_fitness;
	parms.ngeneratedpatches += npatches;
	parms.nselfintersectingpatches += nintersections;


	return true;
}

bool doesMRISselfIntersect(MRIS *mris_work,TOPOFIX_PARMS &parms){
	if(parms.no_self_intersections==false) return false;

	//checking self-intersections
	return IsMRISselfIntersecting(mris_work);
}

extern "C" MRIS *MRISduplicateOver(MRIS *mris,int mode){
	MRIS *mris_dst;
	//clone the surface mris
	mris_dst = MRISclone(mris); 
	for(int n = 0 ; n < mris->nvertices ; n++){
		VERTEX *vdst = &mris_dst->vertices[n];
		VERTEX *vsrc = &mris->vertices[n];
		vdst->marked2=vsrc->marked2;
		vdst->marked = 0 ;
		vdst->ripflag = 0 ;
 		vdst->origx = vsrc->origx;
		vdst->origy =  vsrc->origy;
		vdst->origz =  vsrc->origz;
		vdst->val = vsrc->val;
		vdst->val2 = vsrc->val2;
		vdst->val2bak = vsrc->val2bak;
	}

	int n_extra_vertices,n_extra_faces;

	if(mode==0){
		//count the number of loops to be corrected
		int nloops = (2-MRISgetEuler(mris))/2;

		//count the number of defective vertices
		n_extra_vertices = 0;
		for(int n = 0 ; n < mris->nvertices ; n++)
			if(mris->vertices[n].marked2) n_extra_vertices++;
		n_extra_vertices = 3*n_extra_vertices + nloops*MAX_EXTRA_VERTICES; //2
		
		//count the number of defective faces
		n_extra_faces = 0 ;
		for(int n = 0 ; n < mris->nfaces ; n++){
			bool isface=true;
			for(int i = 0 ; i < 3 ; i++){
				if(mris->vertices[mris->faces[n].v[i]].marked2==0){
					isface = false;
					break;
				}
			}
			if(isface) n_extra_faces++;
		}
		n_extra_faces = 5*n_extra_faces + nloops*MAX_EXTRA_FACES; //4
	}else{
		n_extra_vertices = __MAX(mris->max_vertices-mris->nvertices,0);
		n_extra_faces =__MAX(mris->max_faces-mris->nfaces,0);
	}
	//now allocate the extra space
	VERTEX *vertices = (VERTEX*)calloc(mris->nvertices+n_extra_vertices,sizeof(VERTEX));
  FACE *faces = (FACE*)calloc(mris->nfaces+n_extra_faces,sizeof(FACE));
  for(int n = 0 ; n < mris_dst->nvertices ; n++){
    vertices[n] = mris_dst->vertices[n];
  }
  for(int n = 0 ; n < mris_dst->nfaces ;n++){
    faces[n] = mris_dst->faces[n];
  }
	delete [] mris_dst->vertices;
  mris_dst->vertices=vertices;
  mris_dst->max_vertices = mris_dst->nvertices+n_extra_vertices;
  delete [] mris_dst->faces;
  mris_dst->faces  = faces;
  mris_dst->max_faces = mris_dst->nfaces+n_extra_faces;
	return mris_dst;
}


extern "C" int MRISgetEuler(MRIS *mris, int defect_number){
	int *list_of_faces,nfaces;

	if(defect_number < 0){
		//counting faces
		nfaces = mris->nfaces ;
		if(nfaces == 0) return 0;

		//allocate the list of faces
		list_of_faces = (int*)malloc(nfaces*sizeof(int));

		for(int n = 0 ; n < nfaces ; n++)
			list_of_faces[n]=n;
	}else{

		//counting faces
		nfaces = 0 ;
		for(int n = 0 ; n < mris->nfaces ; n++){
			bool isface=true;
			for(int i = 0 ; i < 3 ; i++){
				if(mris->vertices[mris->faces[n].v[i]].marked2!=defect_number){
					isface=false;
					break;
				}
			}
			if(isface) nfaces++;
		}
		if(nfaces==0) return 0;
		
		//allocate the list of faces
		list_of_faces = (int*)malloc(nfaces*sizeof(int));
		
		//initialize the list of faces
		nfaces = 0 ;
		for(int n = 0 ; n < mris->nfaces ; n++){
			bool isface = true;
			for(int i = 0 ; i < 3 ; i++){
				if(mris->vertices[mris->faces[n].v[i]].marked2!=defect_number){
					isface = false;
					break;
				}
			}
			if(isface) list_of_faces[nfaces++]=n;
		}
	}

	int euler = MRISgetEulerNumber(mris, list_of_faces,nfaces);

	delete [] list_of_faces;

	return euler;
}

extern "C"  int MRISgetEulerNumber(const MRIS *mris, const int *list_of_faces, int nfs){

	int nv,nf,ne;

	//we need to allocate the edge structure for the faces in the list
	//mark and count the vertices
	for(int n = 0 ; n < mris->nvertices ; n++){
		mris->vertices[n].marked = 0;
		mris->vertices[n].e=0;
	}
	nv = 0 ;
	for(int n = 0 ; n < nfs ; n++)
		for(int i = 0 ; i < 3 ; i++){
			VERTEX *v=&mris->vertices[mris->faces[list_of_faces[n]].v[i]];
			if(v->marked==0){
				nv++;
				v->marked=1;
				if(v->e) delete [] v->e; 
				v->e = (int*)calloc(v->vnum,sizeof(int));
			};
		}

	//mark and count the faces
	for(int n = 0 ; n < mris->nfaces ; n++)
		mris->faces[n].marked=0;
	nf=ne=0;
	for(int n = 0 ; n < nfs ; n++){
		FACE *face= &mris->faces[list_of_faces[n]];
		if(face->marked==0){
			nf++;
			face->marked=1;
			int vn0,vn1;
			VERTEX *v0,*v1;
			for(int i = 0 ; i < 3 ; i++){
				vn0 = face->v[i];
				if(i==2) vn1 = face->v[0];
				else vn1 = face->v[i+1];
				v0 = &mris->vertices[vn0];
				v1 = &mris->vertices[vn1];
				//edge vn0 <--> vn1 ?
				for(int p = 0 ; p < v0->vnum ; p++){
					if(v0->v[p] == vn1 && v0->e[p]==0){
						ne++;
						v0->e[p]=1;
						//mark the other edge
						for(int m = 0 ; m < v1->vnum ; m++){
							if(v1->v[m]==vn0){
								v1->e[m]=1;
								break;
							}
						}
						break;
					}
				}
			}
		}
	}
	
	//free the edge structure - reset marks to 0
	for(int n = 0 ; n < nfs ; n++){
		FACE *face= &mris->faces[list_of_faces[n]];
		face->marked = 0;
		for(int i = 0 ; i < 3 ; i++){
			VERTEX *v=&mris->vertices[face->v[i]];
			if(v->marked){
				v->marked=0;
				if(v->e) delete [] v->e;
				v->e=0;
			}
		}
	}
	
	//	fprintf(WHICH_OUTPUT,"(%d,%d,%d)\n",nv,ne,nf);

	return (nv-ne+nf);
}

extern "C" MRIP* MRIPextractFromMRIS(MRIS *mris, int defect_number){
	int *list_of_faces, nfaces;

	/* count the number of faces and vertices */
	//counting the number of vertices
	int nvertices = 0 ; 
	for(int n = 0 ; n < mris->nvertices ; n++)
		if(mris->vertices[n].marked2 == defect_number)
			nvertices++;
	if(nvertices==0) return NULL;
	//counting the number of faces
	nfaces = 0 ;
	for(int n = 0 ; n < mris->nfaces ; n++){
		bool is_face = true;
		FACE *face=&mris->faces[n];
		face->marked=0;
		for(int i = 0 ; i < 3 ; i++)
			if(mris->vertices[face->v[i]].marked2 != defect_number){
				is_face = false;
				break;
			};
		if(is_face) {
			nfaces++;
			face->marked=1;
		}
	}
	list_of_faces = new int[nfaces];
	nfaces=0;
	for(int n = 0 ; n < mris->nfaces ; n++)
		if(mris->faces[n].marked) list_of_faces[nfaces++]=n;

	/* first compute euler number of defect : if one return  0 */
	int euler = MRISgetEulerNumber(mris, list_of_faces, nfaces);
	
	//checking the value of the euler number
	if(euler==1) {
		delete [] list_of_faces;
		return NULL;
	}
	if((euler+1)%2) {
		fprintf(WHICH_OUTPUT,"\n\n Surface non Valid %d\n\n",euler);
		delete [] list_of_faces;
		return NULL;
	}

	//number of loops to be cut
	int ncorrections = (1-euler)/2;

	/* allocate the patch with extra space */
	int max_vertices,max_faces;
//	_OverAlloc(2*loop.npoints+2*pdisk->disk.nvertices,2*(2*loop.npoints+pdisk->disk.nfaces+pdisk->ring.npoints)); 
	max_vertices = nvertices + ncorrections*(2*nvertices + MAX_EXTRA_VERTICES);
	max_faces = nfaces + ncorrections*(4*nvertices+ MAX_EXTRA_FACES);

	MRIP *mrip = MRIPalloc(max_vertices, max_faces);
	mrip->n_vertices = nvertices; // number of vertices before correction
	mrip->n_faces = nfaces; //number of faces before correction

	MRIS *mris_dst=mrip->mris;
	/* copy the necessary information */
	mris_dst->nvertices=nvertices;
	mris_dst->nfaces=nfaces;

	//the corresponding tables
	int *vt_to,*vt_from;
	vt_to = new int[max_vertices];
	for(int n = 0 ; n < max_vertices ; n++)
		vt_to[n]=-1;
	vt_from = new int[mris->nvertices];

	nvertices=0;
	for(int n = 0 ; n < mris->nvertices ; n++){
		vt_from[n]=-1;
		if(mris->vertices[n].marked2 == defect_number){
			VERTEX *vsrc = &mris->vertices[n];
			VERTEX *v = &mris_dst->vertices[nvertices];
			vt_from[n]=nvertices;
			vt_to[nvertices++]=n;
			//copy the strict necessary
			v->x = vsrc->x;
			v->y = vsrc->y;
			v->z = vsrc->z;
		};
	}
	mrip->vtrans_to = vt_to;
	mrip->vtrans_from = vt_from;

	//the corresponding tables
	int *ft_to,*ft_from;
	ft_to = new int[max_faces];
	for(int n = 0 ; n < max_faces ; n++)
		ft_to[n]=-1;
	ft_from = new int[mris->nfaces];

	nfaces=0;
	for(int n = 0 ; n < mris->nfaces ; n++){
		ft_from[n]=-1;
		bool is_face = true;
		for(int i = 0 ; i < 3 ; i++)
			if(mris->vertices[mris->faces[n].v[i]].marked2 != defect_number){
				is_face = false;
				break;
			};
		if(is_face){
			FACE *fsrc = &mris->faces[n];
			FACE *f = &mris_dst->faces[nfaces];
			ft_from[n]=nfaces;
			ft_to[nfaces++]=n;
			for(int i = 0 ; i < 3 ; i++)
				f->v[i] = vt_from[fsrc->v[i]];
		}
	}
	mrip->ftrans_to = ft_to;
	mrip->ftrans_from = ft_from;

	//init the rest
	MRISinitSurface(mris_dst);

	return mrip;
}

void MRISinitSurface(MRIS *mris){
	VERTEX *v;
	
	for(int n = 0 ; n < mris->nvertices ; n++){
		v=&mris->vertices[n];
		v->num=0;
		v->marked=0;
		if(v->f) free(v->f);
    if(v->n) free(v->n);
		if(v->v) free(v->v);
		v->f=NULL;
		v->n=NULL;
		v->v=NULL;
	}

	// counting the number of faces per vertex
	for(int n = 0 ; n < mris->nfaces ; n++)
		for(int i = 0 ; i < 3 ; i++)
			mris->vertices[mris->faces[n].v[i]].num++;
	// allocate the list of faces
	for(int n = 0 ; n < mris->nvertices ; n++){
		VERTEX *v=&mris->vertices[n];
		v->f=(int *)calloc(mris->vertices[n].num,sizeof(int));
		v->n=(uchar *)calloc(mris->vertices[n].num,sizeof(uchar));
		v->num=0;
	}
	// initialize the list of faces
	for(int n = 0 ; n < mris->nfaces ; n++){
		for(int i = 0 ; i < 3 ; i++){
			int vno = mris->faces[n].v[i];
			v=&mris->vertices[vno];
			v->f[v->num]=n;
			v->n[v->num++]=i;
		}
	}

	// counting the list of vertices
	for(int n = 0 ; n < mris->nvertices ; n++){
		v=&mris->vertices[n];
		v->vnum=0;
		for(int p = 0 ; p < v->num ; p++){
			FACE *face=&mris->faces[v->f[p]];
			for( int i = 0 ; i < 3 ; i++){
				int vn=face->v[i];
				if(vn==n) continue;
				if(mris->vertices[vn].marked) continue;
				mris->vertices[vn].marked=1;
				v->vnum++;
			}
		}
		// allocate the list of vertices
		v->v=(int*)calloc(mris->vertices[n].vnum,sizeof(int));
		v->vnum=0;
		for(int p = 0 ; p < v->num ; p++){
			FACE *face=&mris->faces[v->f[p]];
			for( int i = 0 ; i < 3 ; i++){
				int vn=face->v[i];
				if(vn==n) continue;
				if(mris->vertices[vn].marked == 0 ) continue;
				mris->vertices[vn].marked=0;
				v->v[v->vnum++]=vn;
			}
		}
		v->v2num = v->vnum;
		v->v3num = v->vnum;
		v->vtotal = v->vnum;
	}

}


MRIP *MRIPalloc(int nvertices, int nfaces){
	MRIP *patch;

	/* allocate the patch */
	patch = (MRIP*)calloc(1,sizeof(MRIP));
	/* allocate the surface */
	MRIS *mris = MRISoverAlloc(nvertices,nfaces,nvertices,nfaces);

	for(int n = 0 ; n < mris->nvertices ; n++){ //making sure...
		VERTEX *v=&mris->vertices[n];
		v->f=NULL;
		v->n=NULL;
		v->v=NULL;
		v->e=NULL; 
	}
	patch->mris=mris;

	return patch;
}

void MRIPfree(MRIP **mrip){
	MRIP *patch;

	patch = *mrip;
	*mrip = NULL;
	MRIS *mris=patch->mris;

	MRISfree(&mris);

	if(patch->vtrans_to) free(patch->vtrans_to);
	if(patch->ftrans_to) free(patch->ftrans_to);
	if(patch->vtrans_from) free(patch->vtrans_from);
	if(patch->ftrans_from) free(patch->ftrans_from);
	
	free(patch);

}

MRIP *MRIPclone(MRIP *src){
	MRIP *dst;

	/* allocate the patch */
	dst = (MRIP*)calloc(1,sizeof(MRIP));
	/* clone the surface */
	dst->mris = MRISclone(src->mris);
	
	// do not clone the rest
	return dst;
}

Surface *MRIStoSurface(MRIS *mris){
	
	//allocation of the surface
	Surface *surface=new Surface(mris->max_vertices,mris->max_faces);
	surface->nvertices = mris->nvertices;
	surface->nfaces = mris->nfaces;

	for(int n = 0 ; n < mris->nvertices;n++){
		Vertex *vdst = &surface->vertices[n];
		VERTEX *vsrc = &mris->vertices[n];
		vdst->x=vsrc->x;
		vdst->y=vsrc->y;
		vdst->z=vsrc->z;
	}

	for(int n = 0 ; n < mris->nfaces ;n++){
		Face *fdst = &surface->faces[n];
		FACE *fsrc = &mris->faces[n];
		fdst->v[0]=fsrc->v[0];
		fdst->v[1]=fsrc->v[1];
		fdst->v[2]=fsrc->v[2];
	}
	
	surface->InitSurface();

	return surface;
}

void MRIScopyHeader(MRIS *mris_src,MRIS *mris_dst){

	mris_dst->type = mris_src->type;  // missing
  mris_dst->hemisphere = mris_src->hemisphere ;
  mris_dst->xctr = mris_src->xctr ;
  mris_dst->yctr = mris_src->yctr ;
  mris_dst->zctr = mris_src->zctr ;
  mris_dst->xlo = mris_src->xlo ;
  mris_dst->ylo = mris_src->ylo ;
  mris_dst->zlo = mris_src->zlo ;
  mris_dst->xhi = mris_src->xhi ;
  mris_dst->yhi = mris_src->yhi ;
  mris_dst->zhi = mris_src->zhi ;
  mris_dst->min_curv = mris_src->min_curv ;
  mris_dst->max_curv = mris_src->max_curv ;
  mris_dst->total_area = mris_src->total_area ;
  mris_dst->orig_area = mris_src->orig_area ;

  mris_dst->radius = mris_src->radius; // to be checked 

  // just copy the pointer ///////////////////////////////////
#if 0
  mris_dst->linear_transform = mris_src->linear_transform ;
  mris_dst->inverse_linear_transform = mris_src->inverse_linear_transform ;
#endif
  mris_dst->lta = mris_src->lta;
  mris_dst->SRASToTalSRAS_ = mris_src->SRASToTalSRAS_;
  mris_dst->TalSRASToSRAS_ = mris_src->TalSRASToSRAS_;
  mris_dst->free_transform = 0 ;  // mark not to try to free them
  /////////////////////////////////////////////////////////////
	if (mris_src->v_frontal_pole)
      mris_dst->v_frontal_pole = 
				&mris_dst->vertices[mris_src->v_frontal_pole - mris_src->vertices] ;
	if (mris_src->v_occipital_pole)
      mris_dst->v_occipital_pole = 
				&mris_dst->vertices[mris_src->v_occipital_pole - mris_src->vertices] ;
	if (mris_src->v_temporal_pole)
      mris_dst->v_temporal_pole = 
				&mris_dst->vertices[mris_src->v_temporal_pole - mris_src->vertices] ;

	// copy geometry info
	copyVolGeom(&mris_src->vg, &mris_dst->vg);
}

MRIS * SurfaceToMRIS(Surface *surface, MRIS *mris){
	
	if(mris==NULL){//allocate the new surface
		mris=MRISoverAlloc(surface->maxvertices,surface->maxfaces,surface->nvertices,surface->nfaces);
	}
	mris->nvertices=surface->nvertices;
	mris->nfaces=surface->nfaces;
	if(mris->nvertices>=mris->max_vertices)
		fprintf(WHICH_OUTPUT,"too many vertices in mris!\n");
	if(mris->nfaces>=mris->max_faces)
    fprintf(WHICH_OUTPUT,"too many faces in mris!\n");

	//Vertices
	for(int n = 0 ; n < surface->nvertices ; n++){
		VERTEX *vdst=&mris->vertices[n];
		Vertex *vsrc = &surface->vertices[n];
		vdst->x=vsrc->x;
		vdst->y=vsrc->y;
		vdst->z=vsrc->z;
		vdst->ripflag = 0; 
		vdst->marked = 0;
		//vertices
		if(vdst->v) free(vdst->v); vdst->v=NULL;
		vdst->v = (int*)calloc(vsrc->vnum , sizeof(int));
		for(int p = 0 ; p < vsrc->vnum ; p++)
			vdst->v[p]=vsrc->v[p];
		vdst->vnum=vsrc->vnum;
		//faces
		if(vdst->f) free(vdst->f); vdst->f=NULL;
		if(vdst->n) free(vdst->n); vdst->n=NULL;
		vdst->f = (int*)calloc(vsrc->fnum , sizeof(int));
		vdst->n = (uchar*)calloc(vsrc->fnum , sizeof(uchar));
		for(int p = 0 ; p < vsrc->fnum ; p++){
			vdst->f[p]=vsrc->f[p];
			vdst->n[p]=(uchar)vsrc->n[p];
		}
		vdst->num=vsrc->fnum;
	}
	//Faces
	for(int n = 0 ; n < surface->nfaces ; n++){
		FACE *fdst=&mris->faces[n];
		Face *fsrc = &surface->faces[n];
		fdst->v[0]=fsrc->v[0];
		fdst->v[1]=fsrc->v[1];
		fdst->v[2]=fsrc->v[2];
	}

	MRIScomputeNormals(mris);
  MRIScomputeTriangleProperties(mris);
  MRISsaveVertexPositions(mris,ORIGINAL_VERTICES);
	
	return mris;
}


bool MRISaddMRIP(MRIS *mris_dst, MRIP *mrip){
	int nvertices=mrip->n_vertices,nfaces=mrip->n_faces;
	int *vto = mrip->vtrans_to,*fto=mrip->ftrans_to;
	
	int new_vertices = mrip->mris->nvertices - nvertices;
	int new_faces = mrip->mris->nfaces - nfaces;

	if(new_vertices+mris_dst->nvertices>=mris_dst->max_vertices){
		ErrorExit("Not Enough Vertex Space in mris_dst!");
	}
	if(new_faces+mris_dst->nfaces>=mris_dst->max_faces){
    ErrorExit("Not Enough Face Space in mris_dst!");
  }


	MRIS *mris = mrip->mris;

	//vertices
	for(int n = 0 ; n < mris->nvertices ; n++){
		VERTEX *vsrc = &mris->vertices[n];
		if(n >= nvertices) vto[n] = mris_dst->nvertices++;
		VERTEX *vdst = &mris_dst->vertices[vto[n]];
		//coordonnees
		vdst->x=vsrc->x;
		vdst->y=vsrc->y;
		vdst->z=vsrc->z;
	}
	//faces
	for(int n = 0 ; n < mris->nfaces ; n++){
		FACE *fsrc = &mris->faces[n];
		if(n >= nfaces) fto[n] = mris_dst->nfaces++;
		FACE *fdst = &mris_dst->faces[fto[n]];
		//vertex indices
		fdst->v[0]=vto[fsrc->v[0]];
		fdst->v[1]=vto[fsrc->v[1]];
		fdst->v[2]=vto[fsrc->v[2]];
	}
	
	//stuff in vertices and faces
	MRISinitSurface(mris_dst);

	return true;
}
