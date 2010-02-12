/**
 * @file  gcam_mi.cpp
 * @brief morphed GCA
 *
 */
/*
 * Original Author: Gheorghe Postelnicu, MGH, June 2007
 * CVS Revision Info:
 *    $Author: nicks $
 *    $Date: 2010/02/12 19:33:27 $
 *    $Revision: 1.1 $
 *
 * Copyright (C) 2010,
 * The General Hospital Corporation (Boston, MA). 
 * All rights reserved.
 *
 * Distribution, usage and copying of this software is covered under the
 * terms found in the License Agreement file named 'COPYING' found in the
 * FreeSurfer source code root directory, and duplicated here:
 * https://surfer.nmr.mgh.harvard.edu/fswiki/FreeSurferOpenSourceLicense
 *
 * General inquiries: freesurfer@nmr.mgh.harvard.edu
 *
 */

#include <iostream>
#include <vector>

#include <stdlib.h>
#include <time.h>

#include "gcam_mi.h"

struct JointSample
{
  float dFixedIntensity;
  float dMovingIntensity;
};
typedef std::vector<JointSample> JointSampleContainer;

template<class InsertIterator>
void FillContainerWithRandomSamples(GCAM* gcam, MRI* mri,
                                    unsigned int noSamples,
                                    InsertIterator ii);

MRI*
GCAMcomputeMIDenseGradient(GCAM* gcam,
                           MRI* mriMoving)
{
  // just testing
  // do nothing for now - just create a MRI and return

  // build a list of samples
  // do it sequentially for now
  // what container to use?
  // since speed is a concern more than memory -> vector
  JointSampleContainer samples;
  std::cout << " filling the container\n";
  FillContainerWithRandomSamples( gcam, mriMoving,
                                  100000,
                                  std::back_inserter(samples)
                                );
  std::cout << "\t DONE\n";


  MRI* mri = MRIalloc( gcam->atlas.width,
                       gcam->atlas.height,
                       gcam->atlas.depth,
                       MRI_FLOAT);

  return mri;
}


template<class InsertIterator>
void FillContainerWithRandomSamples(GCAM* gcam, MRI* mri,
                                    unsigned int noSamples,
                                    InsertIterator ii)
{
  srand( time(NULL) );

  int x,y,z;
  float dmorphed_x, dmorphed_y, dmorphed_z;

  int width( gcam->width ), height( gcam->height ), depth( gcam->depth );
  JointSample sample;
  GMN* pnode = NULL;
  unsigned int ui=0;
  Real value;

  while ( ui < noSamples )
  {
    // randomly generate an index
    x = rand() % width;
    y = rand() % height;
    z = rand() % depth;

    pnode = &gcam->nodes[x][y][z];

    if ( pnode->invalid == GCAM_POSITION_INVALID ||
         !pnode->gc )
      continue;

    if (GCAMsampleMorph(gcam,
                        x, y, z,
                        &dmorphed_x, &dmorphed_y, &dmorphed_z ) )
      continue;

    MRIsampleVolumeType( mri, dmorphed_x, dmorphed_y, dmorphed_z,
                         &value, SAMPLE_TRILINEAR
                       );


    sample.dFixedIntensity = pnode->gc->means[0];
    sample.dMovingIntensity = value;

    ii = sample;

    // if here, increment ui
    ++ui;
  }
}
