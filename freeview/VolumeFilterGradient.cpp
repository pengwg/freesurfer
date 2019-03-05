/**
 * @file  VolumeFilterGradient.cpp
 * @brief Base VolumeFilterGradient class.
 *
 */
/*
 * Original Author: Ruopeng Wang
 * CVS Revision Info:
 *    $Author: rpwang $
 *    $Date: 2011/10/18 18:13:24 $
 *    $Revision: 1.9 $
 *
 * Copyright © 2011 The General Hospital Corporation (Boston, MA) "MGH"
 *
 * Terms and conditions for use, reproduction, distribution and contribution
 * are found in the 'FreeSurfer Software License Agreement' contained
 * in the file 'LICENSE' found in the FreeSurfer distribution, and here:
 *
 * https://surfer.nmr.mgh.harvard.edu/fswiki/FreeSurferSoftwareLicense
 *
 * Reporting: freesurfer@nmr.mgh.harvard.edu
 *
 *
 */

#include "VolumeFilterGradient.h"
#include <math.h>
#include "LayerMRI.h"
#include <vtkImageData.h>
#include <vtkImageGradientMagnitude.h>
#include <vtkImageShiftScale.h>
#include <vtkImageChangeInformation.h>
#include <vtkImageAnisotropicDiffusion3D.h>
#include <vtkImageGaussianSmooth.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>
#include <vtkImageShiftScale.h>

VolumeFilterGradient::VolumeFilterGradient( LayerMRI* input, LayerMRI* output, QObject* parent ) :
  VolumeFilter( input, output, parent ),
  m_bSmoothing( false ),
  m_dSD( 1.0 )
{
}

bool VolumeFilterGradient::Execute()
{
  TriggerFakeProgress(50);
  vtkSmartPointer<vtkImageGradientMagnitude> grad = vtkSmartPointer<vtkImageGradientMagnitude>::New();
  grad->SetDimensionality( 3 );
  grad->HandleBoundariesOn();

  if ( m_bSmoothing )
  {
    vtkSmartPointer<vtkImageGaussianSmooth> smooth = vtkSmartPointer<vtkImageGaussianSmooth>::New();
    smooth->SetInputData( m_volumeInput->GetImageData() );
    smooth->SetStandardDeviations( m_dSD, m_dSD, m_dSD );
    smooth->SetRadiusFactors( 1, 1, 1 );
    grad->SetInputConnection( smooth->GetOutputPort() );
  }
  else
  {
    grad->SetInputData( m_volumeInput->GetImageData() );
  }

  grad->Update();
  double* orig_range = m_volumeInput->GetImageData()->GetPointData()->GetScalars()->GetRange();
  vtkImageData* img = grad->GetOutput();
  double* range = img->GetPointData()->GetScalars()->GetRange();
  double scale = orig_range[1]/range[1];
  if (scale < 0)
  {
    scale = -scale;
  }
  vtkSmartPointer<vtkImageShiftScale> scaler = vtkSmartPointer<vtkImageShiftScale>::New();
  scaler->SetInputData(img);
  scaler->SetShift(0);
  scaler->SetScale(scale);
  scaler->SetOutputScalarType(m_volumeInput->GetImageData()->GetScalarType());
  scaler->Update();
  m_volumeOutput->GetImageData()->DeepCopy( scaler->GetOutput() );

  return true;
}

