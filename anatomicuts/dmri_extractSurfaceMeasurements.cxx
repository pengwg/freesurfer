/* Alexander Zsikla
 * Professor Siless
 * August 2019
 * dmri_extractSurfaceMeasurements.cxx
 *
 * This program is designed to take in a surface, two overlay files, one or multiple volume files, one or multiple streamline files, and an output directory.
 * Based on the streamlines, output metrics will be placed into a CSV file with the name of the original file and include metrics such as curvature, thickness, and FA values.
 *
 * export FREESURFER_HOME=`readlink -f /home/fsuser2/alex_zsikla/install`
 * source $FREESURFER_HOME/SetUpFreeSurfer.sh
 *
 * ./dmri_extractSurfaceMeasurements -i /home/fsuser2/Desktop/test/AnatomiCuts_long55/1111100011.trk /home/fsuser2/Desktop/test/AnatomiCuts_long55/1111100010.trk /home/fsuser2/Desktop/test/AnatomiCuts_long55/1111011111.trk /home/fsuser2/Desktop/test/AnatomiCuts_long55/1111011110.trk /home/fsuser2/Desktop/test/AnatomiCuts_long55/1101010101.trk /home/fsuser2/Desktop/test/AnatomiCuts_long55/1101010100.trk /home/fsuser2/Desktop/test/AnatomiCuts_long55/1101000111.trk /home/fsuser2/Desktop/test/AnatomiCuts_long55/1101000110.trk
 *
 * ./dmri_extractSurfaceMeasurements -i /home/fsuser2/Desktop/test/AnatomiCuts_long55/1111100011.trk -cl /home/fsuser2/Desktop/test/surf/lh.curv -sl /home/fsuser2/Desktop/test/surf/lh.orig -tl /home/fsuser2/Desktop/test/surf/lh.thickness -cr /home/fsuser2/Desktop/test/surf/rh.curv -sr /home/fsuser2/Desktop/test/surf/rh.orig -tr /home/fsuser2/Desktop/test/surf/rh.thickness -o /home/fsuser2/Desktop/Output_trk -fa 2 FA /home/fsuser2/Desktop/test/dsi_studio/fa.nii.gz AD /home/fsuser2/Desktop/test/dsi_studio/ad.nii.gz 
 *
 */

//Libraries
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

// Input Splicing
#include "GetPot.h"

// TRK Reading
#include <vtkPolyData.h>
#include <vtkPolyDataReader.h>
#include <vtkPolyDataWriter.h>
#include "itkPolylineCell.h"
#include <vtkCellArray.h>
#include <vtkPoints.h>
#include <cmath>
#include "itkArray.h"
#include "itkPolylineCell.h"
#include "TrkVTKPolyDataFilter.txx"
#include "itkImage.h"
#include "PolylineMeshToVTKPolyDataFilter.h"
#include "LabelPerPointVariableLengthVector.h"
#include "EuclideanMembershipFunction.h"
#include "ClusterTools.h"
#include "itkDefaultStaticMeshTraits.h"

// Surface Reading
#include "itkImage.h"
#include <map>
#include "itkDefaultStaticMeshTraits.h"
#include "fsSurface.h"
#include "itkTriangleCell.h"
#include <set>
#include "colortab.h"
#include "fsenv.h"
#include "itkVTKPolyDataWriter.h"
#include "itkSmoothingQuadEdgeMeshFilter.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
	
#include "vtkFillHolesFilter.h" 
#include "vtkPolyDataNormals.h"
#include "vtkCellArray.h"
#include "vtkTriangle.h"
#include "vtkDecimatePro.h"
#include "vtkCleanPolyData.h"
#include "vtkSmoothPolyDataFilter.h"
#include "vtkTriangleFilter.h"

#include "vtkDelaunay3D.h"
#include "macros.h"
#include "mrisurf.h"
#include "mri.h"
#include "vtkKdTreePointLocator.h"
#include "vtkCurvatures.h"

using namespace std;

// HELPER FUNCTIONS
float calculate_mean(vector<float> n);
float calculate_stde(vector<float> n, float mean);
string makeCSV(string dir, string file);
vtkSmartPointer<vtkPolyData> FSToVTK(MRIS* surf);

int main(int narg, char* arg[])
{
	GetPot num1(narg, const_cast<char**>(arg));
	
	// Checking for correct parameters
	if ((num1.size() <= 6) or (num1.search(2, "--help", "-h")))
	{
		cerr << "Usage: " << endl
		     << arg[0] << " -i streamlineFile.trk -sl surfaceFile_lh.orig -tl overlayFile_lh.thickness -cl overlayFile_lh.curv" << endl
		     << "-sr surfaceFile_rh.orig -tr overlayFile_rh.thickness -cr overlayFile_rh.curv -o outputDirectory"<< endl 
		     << "OPTION: -fa <numFiles> <Filename> FA_file.nii.gz ... <Filename> <fileAddress>" << endl;

		return EXIT_FAILURE;
	}

	// Declaration of Variables for Program to Function
	// TRK file Definitions
	enum {Dimension = 3};
	typedef int PixelType;
	const unsigned int PointDimension = 3;
	typedef vector<int> PointDataType;
	const unsigned int MaxTopologicalDimension = 3;
	typedef double CoordinateType;
	typedef double InterpolationWeightType;
	typedef itk::DefaultStaticMeshTraits<
		PointDataType, PointDimension, MaxTopologicalDimension,
		CoordinateType, InterpolationWeightType, PointDataType > MeshTraits;
	typedef itk::Mesh< PixelType, PointDimension, MeshTraits > HistogramMeshType;

	typedef itk::Image<float, 3> ImageType;
	
	typedef itk::Mesh< PixelType, PointDimension > ColorMeshType;
	typedef ColorMeshType::PointType PointType;
	typedef ColorMeshType::CellType CellType;
	typedef itk::PolylineCell<CellType> PolylineCellType;
	typedef ColorMeshType::CellAutoPointer CellAutoPointer;
	typedef ClusterTools<ColorMeshType, ImageType, HistogramMeshType> ClusterToolsType;
	
	// Surface Defintions
	typedef float CoordType;
	typedef fs::Surface< CoordType, Dimension> SurfType;

	// Input Parsing
	vector<string> TRKFiles;	
	for (string inputName = string(num1.follow("", 2, "-i", "-I")); access(inputName.c_str(), 0) == 0; inputName = string(num1.next("")))
		TRKFiles.push_back(inputName);

	// Left Hemisphere
	const char *surfaceFileL = num1.follow("lh.orig", "-sl");
	const char *thickFileL   = num1.follow("lh.thickness", "-tl");
	const char *curvFileL    = num1.follow("lh.curv", "-cl");
	
	// Right Hemisphere
	const char *surfaceFileR = num1.follow("rh.orig", "-sr");
	const char *thickFileR   = num1.follow("rh.thickness", "-tr");
	const char *curvFileR    = num1.follow("rh.curv", "-cr");
	
	const char *outputDir    = num1.follow("measures", "-o");
	
	// Reading in FA file
	vector<ImageType::Pointer> volumes;
	vector<string> image_fileNames;
	
	int numFiles = num1.follow(0, "-fa");
	bool FA_FOUND = num1.search("-fa");
	num1.next("");
	
	if (FA_FOUND and numFiles > 0) 
	{
		for (int i = 0; i < numFiles; i++)
		{
			image_fileNames.push_back(string(num1.next("")));
			const char *inFile = num1.next("");
			typedef itk::ImageFileReader<ImageType> ImageReaderType;
			ImageReaderType::Pointer readerS = ImageReaderType::New();
			readerS->SetFileName(inFile);
			readerS->Update();
			ImageType::Pointer image  = readerS->GetOutput();
			volumes.push_back(image);	
		}
	}

	// Testing that files are saved and can be outputted	
	cerr << endl;
	for (int i = 0; i < TRKFiles.size(); i++)
		cerr << "TRK File " << i + 1 << ":      " << TRKFiles.at(i) << endl;

	cerr << "Left Surface:    " << surfaceFileL << endl << "Left Thickness:  " << thickFileL << endl << "Left Curvature:  " << curvFileL << endl 
	     << "Right Surface:   " << surfaceFileR << endl << "Right Thickness: " << thickFileR << endl << "Right Thickness: " << curvFileR << endl
	     << "Output:          " << outputDir << endl;

	if (FA_FOUND)
	{	
		for (int i = 0; i < image_fileNames.size(); i++)
			cerr << "Image " << i + 1 << ":         " << image_fileNames.at(i) << endl;	
	}

	// Variable Declaration
	ImageType::Pointer mask;	

	vector<ColorMeshType::Pointer>* meshes;
	vector<vtkSmartPointer<vtkPolyData>> polydatas;
	
	ClusterToolsType::Pointer clusterTools = ClusterToolsType::New();
	clusterTools->GetPolyDatas(TRKFiles, &polydatas, mask);
	meshes = clusterTools->PolydataToMesh(polydatas);
	
	//Reading in surface from file
	//For Left Curvature
	MRI_SURFACE *surfCL;
        surfCL = MRISread(surfaceFileL);
	
	SurfType::Pointer surfaceCL = SurfType::New();
	surfaceCL->Load(&*surfCL);
	
	surfCL = surfaceCL->GetFSSurface(&*surfCL);

	MRISreadCurvature(surfCL, curvFileL);	

	//For Left Thickness
	MRI_SURFACE *surfTL;
	surfTL = MRISread(surfaceFileL);

	SurfType::Pointer surfaceTL = SurfType::New();
	surfaceTL->Load(&*surfTL);

	surfTL = surfaceTL->GetFSSurface(&*surfTL);

	MRISreadCurvature(surfTL, thickFileL);

	//For Right Curvature
	MRI_SURFACE *surfCR;
	surfCR = MRISread(surfaceFileR);

	SurfType::Pointer surfaceCR = SurfType::New();
	surfaceCR->Load(&*surfCR);

	surfCR = surfaceCR->GetFSSurface(&*surfCR);

	MRISreadCurvature(surfCR, curvFileR);

	//For Right Thickness
	MRI_SURFACE *surfTR;
	surfTR = MRISread(surfaceFileR);

	SurfType::Pointer surfaceTR = SurfType::New();
	surfaceTR->Load(&*surfTR);

	surfTR = surfaceTR->GetFSSurface(&*surfTR);

	MRISreadCurvature(surfTR, thickFileR);

	// Initializing the KdTree
	// LEFT
	vtkSmartPointer<vtkPolyData> surfVTK_L = FSToVTK(surfCL);
	
	vtkSmartPointer<vtkKdTreePointLocator> surfTreeL = vtkSmartPointer<vtkKdTreePointLocator>::New();
	surfTreeL->SetDataSet(surfVTK_L);
	surfTreeL->BuildLocator();	
	
	// RIGHT
	vtkSmartPointer<vtkPolyData> surfVTK_R = FSToVTK(surfCR);

	vtkSmartPointer<vtkKdTreePointLocator> surfTreeR = vtkSmartPointer<vtkKdTreePointLocator>::New();
	surfTreeR->SetDataSet(surfVTK_R);
	surfTreeR->BuildLocator();	

	PointType firstPt, lastPt;	// The first and last point of a stream
	firstPt.Fill(0);
	lastPt.Fill(0);

	double firstPt_array[3];	// Holding the coordinates of the points
	double lastPt_array[3];

	ofstream oFile;

	// Cycling through the TRK files
	for(int i = 0; i < meshes->size(); i++)
	{ 
		// Opening output file with a different name for every TRK File
		oFile.open(makeCSV(outputDir, TRKFiles.at(i)));

		if (not oFile.is_open()) 
		{
			cerr << "Could not open output file" << endl;
			return -1;
		}

		// Adds the headers to the files and has option for finding FA values
		oFile << "Streamline Name , Curvature of Start Point , Curvature of Last Point , Thickness of Start Point , Thickness of Last Point";
		if (FA_FOUND)
		{
			for (int a = 0; a < image_fileNames.size(); a++)
				oFile << ", meanFA_" << image_fileNames.at(a) << ", stdeFA_" << image_fileNames.at(a);
		} 
		oFile << endl;

		ColorMeshType::Pointer input = (*meshes)[i];
		ColorMeshType::CellsContainer::Iterator  inputCellIt = input->GetCells()->Begin();
		
		// Cycling through the streams
		int counter = 1;
		for (; inputCellIt != input->GetCells()->End(); ++inputCellIt, ++counter)
		{
			vector<float> meanFA;
			vector<float> stdeFA;
		
			// If there are image files, then find the mean and stde of FA	
			if (FA_FOUND)
			{
				for (int p = 0; p < volumes.size(); p++)
				{
					// Creating streamline variable and finding first point
					CellType::PointIdIterator it = inputCellIt.Value()->PointIdsBegin();
					input->GetPoint(*it, &firstPt);

					vector<float> FA_values;
					ImageType::IndexType index;
					if (volumes.at(p)->TransformPhysicalPointToIndex(firstPt, index))
						FA_values.push_back(volumes.at(p)->GetPixel(index));
				
					// Cycling through the points in the stream
					for (; it != inputCellIt.Value()->PointIdsEnd(); it++)
					{	 
						input->GetPoint(*it, &lastPt);

						// If FA options is used, then add the FA values to the vector	
						if (volumes.at(p)->TransformPhysicalPointToIndex(lastPt, index))
							FA_values.push_back(volumes.at(p)->GetPixel(index));	
					}
			
					// Calculating the MeanFA and stdeFA
					meanFA.push_back(calculate_mean(FA_values));
					stdeFA.push_back(calculate_stde(FA_values, meanFA.at(p)));
				}
			// Otherwise find first and last point of a streamline
			} else {
				CellType::PointIdIterator it = inputCellIt.Value()->PointIdsBegin();
				input->GetPoint(*it, &firstPt);

				for (; it != inputCellIt.Value()->PointIdsEnd(); it++)
					input->GetPoint(*it, &lastPt);
			}

			// Copyings points to arrays
			for (int j = 0; j < 3; j++)
		       	{
				firstPt_array[j] = firstPt[j];
				lastPt_array[j]	 = lastPt[j];
			}

			// Finding the vertice number
			double distL, distR;
			vtkIdType Left_ID1  = surfTreeL->FindClosestPointWithinRadius(1000, firstPt_array, distL);
			vtkIdType Right_ID1 = surfTreeR->FindClosestPointWithinRadius(1000, firstPt_array, distR);		
			
			vtkIdType ID1;
			if (distL < distR)
				ID1 = Left_ID1;
			else
				ID1 = Right_ID1;

			vtkIdType Left_ID2  = surfTreeL->FindClosestPointWithinRadius(1000, lastPt_array, distL);
			vtkIdType Right_ID2 = surfTreeR->FindClosestPointWithinRadius(1000, lastPt_array, distR);		
			
			// Comparing the last points distance from the left hemisphere to right hemisphere
			vtkIdType ID2;
			if (distL < distR)
				ID2 = Left_ID2;
			else
				ID2 = Right_ID2;

			// Outputting values to the file
			oFile << "StreamLine " << counter << ",";

			if (ID1 == Left_ID1)
				oFile << surfCL->vertices[ID1].curv << ",";
			else
				oFile << surfCR->vertices[ID1].curv << ",";

			if (ID2 == Left_ID2)
				oFile << surfCL->vertices[ID2].curv << ",";
			else
				oFile << surfCR->vertices[ID2].curv << ",";
			      
			if (ID1 == Left_ID1)
				oFile << surfTL->vertices[ID1].curv << ","; 
			else
				oFile << surfTR->vertices[ID1].curv << ",";

			if (ID2 == Left_ID2)
				oFile << surfTL->vertices[ID2].curv;
			else
				oFile << surfTR->vertices[ID2].curv;
			
			if (FA_FOUND)
			{
				for (int m = 0; m < stdeFA.size(); m++)
					oFile << "," << meanFA.at(m) << "," << stdeFA.at(m);
			}

			oFile << endl;
		}

		oFile.close();
	}
		
	oFile.close();

	return EXIT_SUCCESS;
}

/* Function: makeCSV
 * Input: a directory and a file
 * Returns: a string of a file within the directory
 * Does: Takes in a target directory and returns the directory with the name
 * 	 of the file
 * NOTE: used in conjunction with the creating new CSV files and opening them
 */
string makeCSV(string dir, string file)
{
	int front = file.find_last_of("/");
	int back  = file.find_last_of(".");	
	
	dir.append("/");
	dir.append(file.substr(front + 1, back - front - 1));
	dir.append(".csv");

	return dir;
}

/* Function: calculate_mean
 * Input: a vector of all the FA values
 * Return: the mean
 * Does: takes all the values and calculates the mean
 */
float calculate_mean(vector<float> n)
{
	float mean = 0;

	for (int i = 0; i < n.size(); i++)
		mean += n.at(i);

	return mean / n.size();
}

/* Function: calculate_stde
 * Input: a vector of all the FA values and the mean
 * Return: the standard deviation
 * Does: takes all the values and calculates the standard deviation
 */
float calculate_stde(vector<float> n, float mean)
{
	float SD = 0;

	for (int i = 0; i < n.size(); i++)
		SD += pow(n.at(i) - mean, 2);

	return sqrt(SD / n.size());
}

//
// Converts a surface to a VTK
//
vtkSmartPointer<vtkPolyData> FSToVTK(MRIS* surf)
{
	vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
	vtkSmartPointer<vtkCellArray> triangles = vtkSmartPointer<vtkCellArray>::New();

	for(int i = 0; i < surf->nvertices; i++)
		points->InsertNextPoint(surf->vertices[i].x, surf->vertices[i].y, surf->vertices[i].z);

	for( int i = 0; i < surf->nfaces; i++ ) {
		vtkSmartPointer<vtkTriangle> triangle = vtkSmartPointer<vtkTriangle>::New();
		
		for(int j = 0;j < 3; j++)
			triangle->GetPointIds()->SetId(j, surf->faces[i].v[j]);

		triangles->InsertNextCell(triangle);
	}	

	vtkSmartPointer<vtkPolyData> vtkSurface = vtkSmartPointer<vtkPolyData>::New();
	vtkSurface->SetPoints(points);
	vtkSurface->SetPolys(triangles);

	return vtkSurface;
}

/*
 * Things I Have Changed
 * 1. Commented out some directories that were not recognized
 * 2. Copied directories from freesurfer/resurf/Code to freesurfer/anatomicuts/Code to compile
 */
