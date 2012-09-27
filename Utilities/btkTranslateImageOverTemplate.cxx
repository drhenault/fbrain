/*==========================================================================

  © Université de Strasbourg - Centre National de la Recherche Scientifique

  Date: 12/07/2012
  Author(s): Youssef Taleb, Marc Schweitzer(marc.schweitzer(at)unistra.fr)

  This software is governed by the CeCILL-B license under French law and
  abiding by the rules of distribution of free software.  You can  use,
  modify and/ or redistribute the software under the terms of the CeCILL-B
  license as circulated by CEA, CNRS and INRIA at the following URL
  "http://www.cecill.info".

  As a counterpart to the access to the source code and  rights to copy,
  modify and redistribute granted by the license, users are provided only
  with a limited warranty  and the software's author,  the holder of the
  economic rights,  and the successive licensors  have only  limited
  liability.

  In this respect, the user's attention is drawn to the risks associated
  with loading,  using,  modifying and/or developing or reproducing the
  software by the user in light of its specific status of free software,
  that may mean  that it is complicated to manipulate,  and  that  also
  therefore means  that it is reserved for developers  and  experienced
  professionals having in-depth computer knowledge. Users are therefore
  encouraged to load and test the software's suitability as regards their
  requirements in conditions enabling the security of their systems and/or
  data to be ensured and,  more generally, to use and operate it in the
  same conditions as regards security.

  The fact that you are presently reading this means that you have had
  knowledge of the CeCILL-B license and that you accept its terms.

==========================================================================*/


/* ITK */
#include "itkImage.h"
#include "itkResampleImageFilter.h"
#include "itkAffineTransform.h"
#include "itkNearestNeighborInterpolateImageFunction.h"
#include "itkContinuousIndex.h"
#include "itkImageRegionConstIteratorWithIndex.h"

/* BTK */
#include "btkImageHelper.h"

/* OTHERS */
#include "iostream"
#include "vector"
#include <tclap/CmdLine.h>
#include "fstream"


int main(int argc, char * argv[])
{
    /* Typedefs */

    const unsigned int Dimension = 3;
    typedef itk::Image<short, Dimension>                      Image;
    typedef Image::IndexType                         IndexType;
    typedef itk::ResampleImageFilter<Image,Image>             FilterType;
    typedef itk::AffineTransform< double, Dimension >         TransformType;
    typedef itk::ImageRegionConstIteratorWithIndex< Image >   ConstIteratorType;

    //TCLAP Commands for arguments
    TCLAP::CmdLine cmd("Translate barycenter of input image  on center of the reference image (barycenter is calculated with voxels > 0)", ' ', "Unversioned");
    TCLAP::ValueArg<std::string> inputArg("i","input","Low-resolution image file",true,"","string",cmd);
    TCLAP::ValueArg<std::string> refArg("r","reference","Reference image file",true,"","string",cmd);
    TCLAP::ValueArg<std::string> outArg  ("o","output","Translated image",true,"","string",cmd);
    TCLAP::ValueArg<std::string> matArg  ("t","transform","Transform file (affine transform, with identity rotation and a translation)",true,"","string",cmd);

    std::string  inputFileImage;
    std::string  refFileImage;
    std::string  outputFileImage;

    std::string   translationFileName;
    std::ofstream translationFile;

    // Parse the argv array.
    cmd.parse( argc, argv );
    inputFileImage = inputArg.getValue();
    refFileImage=refArg.getValue();
    outputFileImage = outArg.getValue();
    translationFileName=matArg.getValue();


    //
    // Processing
    //


    try
    {
        // Read reference image
        Image::Pointer refImage=btk::ImageHelper<Image>::ReadImage(refFileImage);

        // Read input image
        Image::Pointer image = btk::ImageHelper<Image>::ReadImage(inputFileImage);

        Image::SizeType size;
        Image::SizeType ref_size;

        size=image->GetLargestPossibleRegion().GetSize();
        ref_size=refImage->GetLargestPossibleRegion().GetSize();

        // Calculate center of reference image in voxel coordinates
        itk::ContinuousIndex< double,Dimension > center_ref,read_point,barycentre;

        for (int i=0;i<=2;i++)
        {
            center_ref[i]=(double)ref_size[i]/2.0;
        }


        // Calculate barycenter of input image
        ConstIteratorType It( image, image->GetLargestPossibleRegion());
        std::cout<<"Processing the barycenter of "<<inputFileImage<<std::flush;
        double x_mean,y_mean,z_mean;
        x_mean = y_mean = z_mean = 0;
        int number_of_points=0;

        for ( It.GoToBegin(); !It.IsAtEnd(); ++It)
        {
            read_point=It.GetIndex();
            if (It.Get()!=0)
            {
                x_mean=x_mean+read_point[0];
                y_mean=y_mean+read_point[1];
                z_mean=z_mean+read_point[2];
                ++number_of_points;
            }
        }

        x_mean=x_mean/number_of_points;
        y_mean=y_mean/number_of_points;
        z_mean=z_mean/number_of_points;

        barycentre[0]=x_mean;
        barycentre[1]=y_mean;
        barycentre[2]=z_mean;


        Image::PointType pi;
        image->TransformContinuousIndexToPhysicalPoint(barycentre,pi);

        Image::PointType pr;
        refImage->TransformContinuousIndexToPhysicalPoint(center_ref,pr);

        std::cout<<"Apply translation..."<<std::flush;
        // Processing Translation
        FilterType::Pointer filter = FilterType::New();
        TransformType::Pointer transform = TransformType::New();

        TransformType::OutputVectorType translation;
        translation=pi-pr;
        transform->Translate( translation );

        //std::cout<<transform;

        filter->UseReferenceImageOn();
        filter->SetReferenceImage(refImage);
        filter->SetInput( image );
        filter->SetTransform( transform );
        filter->Update();

        std::cout<<"done !"<<std::endl;

        //Write ouput image

        btk::ImageHelper<Image>::WriteImage(filter->GetOutput() ,outputFileImage);

        // Write Transform file
        //TODO: If We Set a itk::AffineTransform<double, 3> with a identity and we set translation to the SetOffset function we should have the same result
        translationFile.open(translationFileName.c_str(), std::ios::out);
        translationFile <<"Transform : MatrixOffsetTransformBase_double_3_3" << std::endl;
        translationFile <<"Parameters: 1 0 0 0 1 0 0 0 1 "<<translation[0]<<" " << translation[1]<<" "<<translation[2]<< std::endl;
        translationFile <<"FixedParameters: 0 0 0" << std::endl;
        translationFile.close();

    }
    catch(itk::ExceptionObject &error)
    {
        std::cout << "ITK error: " << error << std::endl;
        return EXIT_FAILURE;
    }
    catch(std::string &message)
    {
        std::cout << "Error: " << message << std::endl;
        return EXIT_FAILURE;
    }

    std::cout<<"End of execution..."<<std::endl;

    return EXIT_SUCCESS;

}
