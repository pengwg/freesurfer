/**
 * @file  vtkRGBAColorTransferFunction.cxx
 * @brief Defines transfer function for mapping a property to an RGBA color value
 *
 * This code is based on vtkColorTransferFunction. It was modified to
 * add an alpha element to all color output. This required adding a
 * 5th element to the Function units, and changing all Add***Point and
 * Add***Segment functions to Add***APoint and Add***ASegment, adding
 * an alpha parameter to each.
 */
/*
 * Original Author: Kitware, Inc, modified by Ruopeng Wang
 * CVS Revision Info:
 *    $Author: nicks $
 *    $Date: 2011/03/02 00:04:56 $
 *    $Revision: 1.2 $
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
 */

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkRGBAColorTransferFunction.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRGBAColorTransferFunction - Defines a transfer function for mapping a property to an RGB color value.

// .SECTION Description
// vtkRGBAColorTransferFunction is a color mapping in RGB or HSV space that
// uses piecewise hermite functions to allow interpolation that can be
// piecewise constant, piecewise linear, or somewhere in-between 
// (a modified piecewise hermite function that squishes the function
// according to a sharpness parameter). The function also allows for
// the specification of the midpoint (the place where the function
// reaches the average of the two bounding nodes) as a normalize distance
// between nodes.

// .SECTION see also
// vtkPiecewiseFunction

#ifndef __vtkRGBAColorTransferFunction_h
#define __vtkRGBAColorTransferFunction_h

#include "vtkScalarsToColors.h"

class vtkRGBAColorTransferFunctionInternals;

#define VTK_CTF_RGB           0
#define VTK_CTF_HSV           1
#define VTK_CTF_LAB           2
#define VTK_CTF_DIVERGING     3

#define VTK_CTF_LINEAR        0
#define VTK_CTF_LOG10         1

class VTKCOMMONCORE_EXPORT vtkRGBAColorTransferFunction : public vtkScalarsToColors
{
public:
  static vtkRGBAColorTransferFunction *New();
  vtkTypeMacro(vtkRGBAColorTransferFunction,vtkScalarsToColors);
  void DeepCopy( vtkRGBAColorTransferFunction *f );
  void ShallowCopy( vtkRGBAColorTransferFunction *f );

  // Description:
  // Print method for vtkRGBAColorTransferFunction
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // How many points are there defining this function?
  int GetSize();
  
  // Description:
  // Add/Remove a point to/from the function defined in RGB or HSV
  // Return the index of the point (0 based), or -1 on error.
  int AddRGBAPoint( double x, double r, double g, double b, double a );
  int AddRGBAPoint( double x, double r, double g, double b, double a,
                   double midpoint, double sharpness );
  int AddHSVAPoint( double x, double h, double s, double v, double a );
  int AddHSVAPoint( double x, double h, double s, double v, double a,
                   double midpoint, double sharpness );
  int RemovePoint( double x );

  // Description:
  // Add two points to the function and remove all the points 
  // between them
  void AddRGBASegment( double x1, double r1, double g1, double b1, double a1,
                      double x2, double r2, double g2, double b2, double a2 );
  void AddHSVASegment( double x1, double h1, double s1, double v1, double a1,
                      double x2, double h2, double s2, double v2, double a2 );
  
  // Description:
  // Remove all points
  void RemoveAllPoints();

  // Description:
  // Returns an RGBA color for the specified scalar value 
  double *GetColor(double x) {
    return vtkScalarsToColors::GetColor(x); }
  void GetColor(double x, double rgba[4]);

  // Description:
  // Get the color components individually.
  double GetRedValue( double x );
  double GetGreenValue( double x );
  double GetBlueValue( double x );
  double GetAlphaValue( double x );

  // Description:
  // For the node specified by index, set/get the
  // location (X), R, G, B and A values, midpoint, and 
  // sharpness values at the node.
  int GetNodeValue( int index, double val[7] );
  int SetNodeValue( int index, double val[7] );
  
  // Description:
  // Map one value through the lookup table.
  virtual unsigned char *MapValue(double v);

  // Description:
  // Returns min and max position of all function points.
  vtkGetVector2Macro( Range, double );

  // Description:
  // Remove all points out of the new range, and make sure there is a point
  // at each end of that range.
  // Return 1 on success, 0 otherwise.
  int AdjustRange(double range[2]);

  // Description:
  // Fills in a table of n function values between x1 and x2
  void GetTable( double x1, double x2, int n, double* table );
  void GetTable( double x1, double x2, int n, float* table );
  const unsigned char *GetTable( double x1, double x2, int n);

  // Description:
  // Construct a color transfer function from a table. Function range is
  // is set to [x1, x2], each function size is set to size, and function 
  // points are regularly spaced between x1 and x2. Parameter "table" is 
  // assumed to be a block of memory of size [4*size] (RGBA)
  void BuildFunctionFromTable( double x1, double x2, int size, double *table);

  // Description:
  // Sets and gets the clamping value for this transfer function.
  vtkSetClampMacro( Clamping, int, 0, 1 );
  vtkGetMacro( Clamping, int );
  vtkBooleanMacro( Clamping, int );
  
  // Description:
  // Set/Get the color space used for interpolation: RGB, HSV, CIELAB, or
  // Diverging.  In HSV mode, if HSVWrap is on, it will take the shortest path
  // in Hue (going back through 0 if that is the shortest way around the hue
  // circle) whereas if HSVWrap is off it will not go through 0 (in order the
  // match the current functionality of vtkLookupTable).  Diverging is a special
  // mode where colors will pass through white when interpolating between two
  // saturated colors.
  vtkSetClampMacro( ColorSpace, int, VTK_CTF_RGB, VTK_CTF_DIVERGING );
  void SetColorSpaceToRGB(){this->SetColorSpace(VTK_CTF_RGB);};
  void SetColorSpaceToHSV(){this->SetColorSpace(VTK_CTF_HSV);};
  void SetColorSpaceToLab(){this->SetColorSpace(VTK_CTF_LAB);};
  void SetColorSpaceToDiverging(){this->SetColorSpace(VTK_CTF_DIVERGING);}
  vtkGetMacro( ColorSpace, int );
  vtkSetMacro(HSVWrap, int);
  vtkGetMacro(HSVWrap, int);
  vtkBooleanMacro(HSVWrap, int);

  // Description:
  // Set the type of scale to use, linear or logarithmic.  The default
  // is linear.  If the scale is logarithmic, and the range contains
  // zero, the color mapping will be linear.
  vtkSetMacro(Scale,int);
  void SetScaleToLinear() { this->SetScale(VTK_CTF_LINEAR); };
  void SetScaleToLog10() { this->SetScale(VTK_CTF_LOG10); };
  vtkGetMacro(Scale,int);
    
  // Description:
  // Returns a list of all nodes
  // Fills from a pointer to data stored in a similar list of nodes.
  double *GetDataPointer();
  void FillFromDataPointer(int, double*);

  // Description:
  // map a set of scalars through the lookup table
  virtual void MapScalarsThroughTable2(void *input, unsigned char *output,
                                     int inputDataType, int numberOfValues,
                                     int inputIncrement, int outputIncrement);
  
  // Description:
  // Toggle whether to allow duplicate scalar values in the color transfer
  // function (off by default).
  vtkSetMacro(AllowDuplicateScalars, int);
  vtkGetMacro(AllowDuplicateScalars, int);
  vtkBooleanMacro(AllowDuplicateScalars, int);

protected:
  vtkRGBAColorTransferFunction();
  ~vtkRGBAColorTransferFunction();

  vtkRGBAColorTransferFunctionInternals *Internal;
  
  // Determines the function value outside of defined points
  // Zero = always return 0.0 outside of defined points
  // One  = clamp to the lowest value below defined points and
  //        highest value above defined points
  int Clamping;

  // The color space in which interpolation is performed
  int ColorSpace;

  // Specify if HSW is warp or not
  int HSVWrap;

  // The color interpolation scale (linear or logarithmic).
  int Scale;
  
  double     *Function;
  
  // The min and max node locations
  double Range[2];
  
  // An evaluated color (0 to 255 RGBA)
  unsigned char UnsignedCharRGBAValue[4];

  int AllowDuplicateScalars;

  vtkTimeStamp BuildTime;
  unsigned char *Table;
  int TableSize;
  
  // Description:
  // Set the range of scalars being mapped. The set has no functionality
  // in this subclass of vtkScalarsToColors.
  virtual void SetRange(double, double) {};
  void SetRange(double rng[2]) {this->SetRange(rng[0],rng[1]);};

  // Internal method to sort the vector and update the
  // Range whenever a node is added or removed
  void SortAndUpdateRange();
 
  // Description:
  // Moves point from oldX to newX. It removed the point from oldX. If any point
  // existed at newX, it will also be removed.
  void MovePoint(double oldX, double newX);

private:
  vtkRGBAColorTransferFunction(const vtkRGBAColorTransferFunction&);  // Not implemented.
  void operator=(const vtkRGBAColorTransferFunction&);  // Not implemented.
};

#endif

