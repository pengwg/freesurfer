import logging
import math
import operator
from functools import reduce

import GEMS2Python
import numpy as np
from scipy import ndimage

from as_python.samseg.color_scheme import ColorScheme
from as_python.samseg.kvl import transform_product, calculate_down_sampling_factors

logger = logging.getLogger(__name__)

# K = 1e-7; % Mesh stiffness -- compared to normal models, the entropy cost function is normalized
#           % (i.e., measures an average *per voxel*), so that this needs to be scaled down by the
#           % number of voxels that are covered
#   scaling = 0.9 * ones( 1, 3 );
#   K = K / prod( scaling );

MESH_STIFFNESS = 1e-7
SCALING_FACTOR = 0.9
SCALING_FACTOR_CUBED = SCALING_FACTOR * SCALING_FACTOR * SCALING_FACTOR
SCALED_MESH_STIFFNESS = MESH_STIFFNESS / SCALING_FACTOR_CUBED

# targetDownsampledVoxelSpacing = 3.0; % In mm
TARGET_DOWNSAMPLE_VOXEL_SPACING = 3.0

USE_SIMPLE_MESH_POSITIONING = True
ELIMINATE_BACKGROUND_CLASS = False

# priorVisualizationAlpha = 0.4;
PRIOR_VISUALIZATION_ALPHA = 0.4

MAXIMAL_DEFORMATION_STOP_CRITERIA = 0.005  # Measured in voxels


def DEPRECATED_samseg_register_atlas(recipe):
    #         image_file_name,
    #         mesh_collection_file_name,
    #         template_file_name,
    #         save_path,
    #         show_figures
    # ):
    # function worldToWorldTransformMatrix = samseg_registerAtlas( imageFileName, meshCollectionFileName, templateFileName, savePath, showFigures )
    #
    # %
    #
    logger.info('Begin atlas registration')
    original_image = read_input_image_with_transform(recipe.image_file_name)
    original_image_to_world_transform = original_image.transform_matrix

    template = read_input_image_with_transform(recipe.template_file_name)
    template_image_to_world_transform = template.transform_matrix

    initial_world_to_world_transform_matrix = determine_initial_world_to_world_transform_matrix()
    initial_image_to_image_transform_matrix = determine_initial_image_to_image_transform_matrix(
        initial_world_to_world_transform_matrix,
        template_image_to_world_transform
    )

    down_sampling_factors = calculate_down_sampling_factors(
        original_image_to_world_transform, TARGET_DOWNSAMPLE_VOXEL_SPACING)
    logger.info('down sampling is %s', str(down_sampling_factors))

    mesh = determine_mesh(
        down_sampling_factors,
        recipe.mesh_collection_file_name,
        initial_image_to_image_transform_matrix,
        original_image_to_world_transform,
        template_image_to_world_transform
    )

    down_sampled_image = down_sample_image(original_image, down_sampling_factors, recipe.show_figures)
    mesh_down_scaling_factor = [1.0 / down_sample_factor for down_sample_factor in down_sampling_factors]
    mesh.scale(mesh_down_scaling_factor)

    color_scheme = handle_background_class_and_determine_color_scheme(mesh)
    if recipe.show_figures:
        show_mesh(mesh, down_sampled_image, color_scheme)

    calculator = create_calculator(down_sampled_image)

    initial_image_to_image_transform_matrix = find_low_cost_initial_mesh_position(
        calculator,
        mesh,
        down_sampled_image,
        initial_image_to_image_transform_matrix,
        down_sampling_factors
    )

    # %
    # originalNodePositions = kvlGetMeshNodePositions( mesh );
    #
    original_node_positions = mesh.points

    if recipe.show_figures:
        show_starting_situation(mesh, down_sampled_image, color_scheme)

    optimizer = get_optimizer(mesh, calculator)

    # tic
    number_of_iterations = perform_optimization(
        optimizer,
        mesh,
        original_node_positions,
        color_scheme,
        recipe.show_figures
    )
    # toc

    world_to_world_transform_matrix = save_results(
        down_sampling_factors,
        down_sampled_image,
        recipe.image_file_name,
        initial_image_to_image_transform_matrix,
        mesh,
        original_node_positions,
        recipe.save_path,
        recipe.template_file_name
    )
    logger.info('Completed atlas registration')
    return world_to_world_transform_matrix


def read_input_image_with_transform(image_file_name):
    # [ image, imageToWorldTransform ] = kvlReadImage( imageFileName );
    # imageToWorldTransformMatrix = double( kvlGetTransformMatrix( imageToWorldTransform ) );
    # [ template, templateImageToWorldTransform ] = kvlReadImage( templateFileName );
    # templateImageToWorldTransformMatrix = double( kvlGetTransformMatrix( templateImageToWorldTransform ) );
    logger.info("reading image %s", image_file_name)
    return GEMS2Python.KvlImage(image_file_name)


def determine_initial_world_to_world_transform_matrix():
    # initialWorldToWorldTransformMatrix = eye( 4 );
    # if true
    #   % Provide an initial (non-identity) affine transform guestimate
    #
    #   % Rotation around X-axis (direction from left to right ear)
    #   theta = pi/180 * -10.0;
    #   rotationMatrix = eye( 4 );
    #   rotationMatrix( 2 : 3, 2 : 3 ) = [ cos( theta ) -sin(theta); sin(theta) cos( theta ) ];
    #   initialWorldToWorldTransformMatrix = rotationMatrix * initialWorldToWorldTransformMatrix;
    #
    #   % Isotropic scaling
    #   scaling = 0.9 * ones( 1, 3 );
    #   scalingMatrix = diag( [ scaling 1 ] );
    #
    #   initialWorldToWorldTransformMatrix = scalingMatrix * initialWorldToWorldTransformMatrix;
    #
    #   K = K / prod( scaling );
    # end

    #   % Rotation around X-axis (direction from left to right ear)
    theta = math.pi / 180 * 10.0
    cos_theta = math.cos(theta)
    sin_theta = math.sin(theta)
    rotation_matrix = np.identity(4, dtype=np.double)
    rotation_matrix[2, 2] = cos_theta
    rotation_matrix[2, 3] = -sin_theta
    rotation_matrix[3, 2] = sin_theta
    rotation_matrix[3, 3] = cos_theta

    scaling_matrix = np.diag([SCALING_FACTOR, SCALING_FACTOR, SCALING_FACTOR, 1.0])

    scaled_and_rotated = np.dot(scaling_matrix, rotation_matrix)

    initial_world_transformation_matrix = GEMS2Python.KvlTransform(scaled_and_rotated)
    return initial_world_transformation_matrix


def determine_initial_image_to_image_transform_matrix(
        initial_world_to_world_transform_matrix, template_image_to_world_transform):
    # % Read in image, and figure out where the mesh is located
    #
    #
    # initialImageToImageTransformMatrix = imageToWorldTransformMatrix \ ...
    #                   ( initialWorldToWorldTransformMatrix * templateImageToWorldTransformMatrix );
    return transform_product(initial_world_to_world_transform_matrix, template_image_to_world_transform)


def determine_mesh(
        down_sampling_factors,
        mesh_collection_file_name,
        initial_image_to_image_transform_matrix,
        original_image_to_world_transform,
        template_image_to_world_transform
):
    if USE_SIMPLE_MESH_POSITIONING:
        mesh = determine_mesh_simply(
            down_sampling_factors,
            mesh_collection_file_name,
            initial_image_to_image_transform_matrix
        )
    else:
        mesh = determine_mesh_properly(
            down_sampling_factors,
            mesh_collection_file_name,
            original_image_to_world_transform,
            template_image_to_world_transform
        )
    # kvlScaleMesh( mesh, 1 ./ downSamplingFactors );
    return mesh


def determine_mesh_simply(
        down_sampling_factors,
        mesh_collection_file_name,
        initial_image_to_image_transform_matrix
):
    #   % Use initial transform to define the reference (rest) position of the mesh (i.e., the one
    #   % where the log-prior term is zero)
    #   meshCollection = kvlReadMeshCollection( meshCollectionFileName, ...
    #                                           kvlCreateTransform( initialImageToImageTransformMatrix ), ...
    #                                           K * prod( downSamplingFactors ) );
    #   mesh = kvlGetMesh( meshCollection, -1 );
    mesh_collection = GEMS2Python.KvlMeshCollection()
    mesh_collection.read(mesh_collection_file_name)
    mesh_collection.k = reduce(operator.mul, down_sampling_factors, SCALED_MESH_STIFFNESS)
    mesh_collection.transform(initial_image_to_image_transform_matrix)
    mesh = mesh_collection.reference_mesh
    return mesh


def determine_mesh_properly(
        down_sampling_factors,
        mesh_collection_file_name,
        image_to_world_transform_matrix,
        template_image_to_world_transform_matrix
):
    #   % "Proper" initialization: apply the initial transform but don't let it affect the deformation
    #   % prior
    #   meshCollection = kvlReadMeshCollection( meshCollectionFileName, ...
    #                                           kvlCreateTransform( imageToWorldTransformMatrix \ ...
    #                                                               templateImageToWorldTransformMatrix ), ...
    #                                           K * prod( downSamplingFactors ) );
    #   mesh = kvlGetMesh( meshCollection, -1 );
    #   nodePositions = kvlGetMeshNodePositions( mesh );
    #   tmp = [ nodePositions ones( size( nodePositions, 1 ), 1 ) ];
    #   tmp = ( imageToWorldTransformMatrix \ ...
    #           ( initialWorldToWorldTransformMatrix * imageToWorldTransformMatrix ) ) * tmp';
    #   nodePositions = tmp( 1:3, : )';
    #   kvlSetMeshNodePositions( mesh, nodePositions );
    pass


def down_sample_image(original_image, down_sampling_factors, show_figures):
    # % Get image data
    # imageBuffer = kvlGetImageBuffer( image );
    # if showFigures
    #   figure
    #   showImage( imageBuffer );
    # end
    #
    # % Downsample
    # imageBuffer = imageBuffer( 1 : downSamplingFactors( 1 ) : end, ...
    #                            1 : downSamplingFactors( 2 ) : end, ...
    #                            1 : downSamplingFactors( 3 ) : end );
    # image = kvlCreateImage( imageBuffer );
    image_buffer = original_image.getImageBuffer()
    [down_x, down_y, down_z] = down_sampling_factors
    down_sampled_image_buffer = np.asfortranarray(image_buffer[::down_x, ::down_y, ::down_z])
    return GEMS2Python.KvlImage(down_sampled_image_buffer)


def handle_background_class_and_determine_color_scheme(mesh):
    #
    # alphas = kvlGetAlphasInMeshNodes( mesh );
    alphas = mesh.alphas
    [number_of_classes, _] = alphas.shape
    # gmClassNumber = 3;  % Needed for displaying purposes
    gm_class_number = 3
    # if 0
    #   % Get rid of the background class
    #   alphas = alphas( :, 2 : end );
    #   kvlSetAlphasInMeshNodes( mesh, alphas )
    #   gmClassNumber = gmClassNumber-1;
    # end
    # numberOfClasses = size( alphas, 2 );
    # colors = 255 * [ hsv( numberOfClasses ) ones( numberOfClasses, 1 ) ];
    #
    return ColorScheme(number_of_classes, gm_class_number)


def show_mesh(mesh, image, color_scheme):
    #
    # if showFigures
    #   figure
    #   priors = kvlRasterizeAtlasMesh( mesh, size( imageBuffer ) );
    #   for classNumber = 1 : numberOfClasses
    #     subplot( 2, 3, classNumber )
    #     showImage( priors( :, :, :, classNumber ) )
    #   end
    # end
    pass


def create_calculator(image):
    #
    # %
    # % Get a registration cost and use it to evaluate some promising starting point proposals
    # calculator = kvlGetCostAndGradientCalculator( 'MutualInformation', ...
    #                                                image, 'Affine' );
    return GEMS2Python.KvlCostAndGradientCalculator('MutualInformation', [image], 'Affine')


def find_low_cost_initial_mesh_position(
        calculator,
        mesh,
        image,
        initial_image_to_image_transform_matrix,
        down_sampling_factors
):
    # [ cost gradient ] = kvlEvaluateMeshPosition( calculator, mesh );
    # if true
    #   %
    #   [ xtmp, ytmp, ztmp ] = ndgrid( 1 : size( imageBuffer, 1 ), ...
    #                                  1 : size( imageBuffer, 2 ), ...
    #                                  1 : size( imageBuffer, 3 ) );
    #   centerOfGravityImage = [ xtmp(:) ytmp(:) ztmp(:) ]' * imageBuffer(:) / sum( imageBuffer(:) );
    #
    #   priors = kvlRasterizeAtlasMesh( mesh, size( imageBuffer ) );
    #   %tmp = sum( priors, 4 );
    #   tmp = sum( priors( :, :, :, 2 : end ), 4 );
    #   centerOfGravityAtlas = [ xtmp(:) ytmp(:) ztmp(:) ]' * tmp(:) / sum( tmp(:) );
    #
    #   %
    #   initialTranslation = double( centerOfGravityImage - centerOfGravityAtlas );
    #   nodePositions = kvlGetMeshNodePositions( mesh );
    #   trialNodePositions = nodePositions + repmat( initialTranslation', [ size( nodePositions, 1 ) 1 ] );
    #   kvlSetMeshNodePositions( mesh, trialNodePositions );
    #   [ trialCost trialGradient ] = kvlEvaluateMeshPosition( calculator, mesh );
    #   if ( trialCost >= cost )
    #     % Center of gravity was not a success; revert to what we had before
    #     kvlSetMeshNodePositions( mesh, nodePositions );
    #   else
    #     % This is better starting position; remember that we applied it
    #     initialImageToImageTransformMatrix( 1 : 3, 4 ) = ...
    #              initialImageToImageTransformMatrix( 1 : 3, 4 ) + diag( downSamplingFactors ) * initialTranslation;
    #   end
    #
    # end
    #
    image_buffer = image.getImageBuffer()
    initial_positions = mesh.points
    initial_cost, _ = calculator.evaluate_mesh_position(mesh)
    logger.info("initial_cost=%f", initial_cost)

    translation = center_of_gravity_adjusted_translation(mesh, image_buffer)
    translated_positions = initial_positions + translation
    mesh.points = translated_positions

    trial_cost, _ = calculator.evaluate_mesh_position(mesh)
    logger.info("trial_cost=%f", trial_cost)

    if trial_cost > initial_cost:
        # revert positions and return unchanged transform
        logger.info('keeping original position')
        mesh.points = initial_positions
        return initial_image_to_image_transform_matrix
    else:
        # Keep this better postion and remember that we applied it
        logger.info('updating position via center of gravity matching')
        transform_matrix = initial_image_to_image_transform_matrix.as_numpy_array
        for axis in range(3):
            transform_matrix[3, axis] += down_sampling_factors[axis] * translation[axis]
        return GEMS2Python.KvlTransform(transform_matrix)


def center_of_gravity_adjusted_translation(mesh, image_buffer):
    shape = image_buffer.shape
    [image_x, image_y, image_z] = ndimage.measurements.center_of_mass(image_buffer)
    logger.info("center of gravity for image = [%f, %f, %f]", image_x, image_y, image_z)
    [atlas_x, atlas_y, atlas_z] = center_of_gravity_of_mesh(mesh, shape)
    logger.info("center of gravity for mesh = [%f, %f, %f]", atlas_x, atlas_y, atlas_z)
    return [
        image_x - atlas_x,
        image_y - atlas_y,
        image_z - atlas_z,
    ]


def center_of_gravity_of_mesh(mesh, shape):
    priors_buffer = mesh.rasterize(shape)
    non_background_summation = priors_buffer[:, :, :, 2:].sum(axis=3)
    result = ndimage.measurements.center_of_mass(non_background_summation)
    return result


def show_starting_situation(mesh, image_buffer, color_scheme):
    # % Visualize starting situation
    # priorVisualizationAlpha = 0.4;
    # if showFigures
    #   figure
    #
    #   priors = kvlRasterizeAtlasMesh( mesh, size( imageBuffer ) );
    #   colorCodedPriors = kvlColorCodeProbabilityImages( priors, colors );
    #   mask = ( sum( double( priors ), 4 ) / (2^16-1) ) > .5;
    #   subplot( 2, 2, 1 )
    #   showImage( imageBuffer );
    #   subplot( 2, 2, 2 )
    #   showImage( imageBuffer .* mask );
    #   subplot( 2, 2, 3 )
    #   imageToShow = ( imageBuffer - min( imageBuffer(:) ) ) / ( max( imageBuffer(:) ) - min( imageBuffer(:) ) );
    #   imageToShow = ( 1 - priorVisualizationAlpha ) * repmat( imageToShow, [ 1 1 1 3 ] ) + ...
    #                 priorVisualizationAlpha * colorCodedPriors;
    #   showImage( imageToShow )
    #   subplot( 2, 2, 4 )
    #   tmp = double( priors( :, :, :, gmClassNumber ) ) / ( 2^16-1 );
    #   showImage( mosaicImages( tmp, imageBuffer .* mask, 2 ) );
    #
    #   drawnow
    # end
    pass


def get_optimizer(mesh, calculator):
    # lineSearchMaximalDeformationIntervalStopCriterion = maximalDeformationStopCriterion; % Doesn't seem to matter very much
    #
    # % Get an optimizer, and stick the cost function into it
    # optimizerType = 'L-BFGS';
    # optimizer = kvlGetOptimizer( optimizerType, mesh, calculator, ...
    #                                 'Verbose', 1, ...
    #                                 'MaximalDeformationStopCriterion', maximalDeformationStopCriterion, ...
    #                                 'LineSearchMaximalDeformationIntervalStopCriterion', ...
    #                                 lineSearchMaximalDeformationIntervalStopCriterion, ...
    #                                 'BFGS-MaximumMemoryLength', 12 ); % Affine registration only has 12 DOF
    #
    optimization_parameters = {
        'Verbose': 1.0,
        'MaximalDeformationStopCriterion': MAXIMAL_DEFORMATION_STOP_CRITERIA,
        'LineSearchMaximalDeformationIntervalStopCriterion': MAXIMAL_DEFORMATION_STOP_CRITERIA,
        'BFGS-MaximumMemoryLength': 12.0  # Affine registration only has 12 DOF
    }
    logger.info("optimization parameters = %s", str(optimization_parameters))
    return GEMS2Python.KvlOptimizer(
        'L-BFGS',
        mesh,
        calculator,
        optimization_parameters
    )


def perform_optimization(
        optimizer,
        mesh,
        original_node_positions,
        color_scheme,
        show_figures
):
    # while true
    #   %
    #   [ minLogLikelihoodTimesPrior, maximalDeformation ] = kvlStepOptimizer( optimizer )
    #   %return
    #   if ( maximalDeformation == 0 )
    #     break;
    #   end
    #   numberOfIterations = numberOfIterations + 1;
    #
    #   %
    #   % Visualize progress
    #   %
    #   if showFigures
    #     % Show figure
    #     priors = kvlRasterizeAtlasMesh( mesh, size( imageBuffer ) );
    #     colorCodedPriors = kvlColorCodeProbabilityImages( priors, colors );
    #     mask = ( sum( double( priors ), 4 ) / (2^16-1) ) > .5;
    #     subplot( 2, 2, 1 )
    #     showImage( imageBuffer );
    #     subplot( 2, 2, 2 )
    #     showImage( imageBuffer .* mask );
    #     subplot( 2, 2, 3 )
    #     imageToShow = ( imageBuffer - min( imageBuffer(:) ) ) / ( max( imageBuffer(:) ) - min( imageBuffer(:) ) );
    #     imageToShow = ( 1 - priorVisualizationAlpha ) * repmat( imageToShow, [ 1 1 1 3 ] ) + ...
    #                   priorVisualizationAlpha * colorCodedPriors;
    #     showImage( imageToShow )
    #     subplot( 2, 2, 4 )
    #     tmp = double( priors( :, :, :, gmClassNumber ) ) / ( 2^16-1 );
    #     showImage( mosaicImages( tmp, imageBuffer .* mask, 2 ) );
    #     drawnow
    #
    #     % Show affine matrix, retrieved from any four non-colinear points before and after registration
    #     nodePositions = kvlGetMeshNodePositions( mesh );
    #     pointNumbers = [ 1 111 202 303 ];
    #     originalY = [ originalNodePositions( pointNumbers, : )'; 1 1 1 1 ];
    #     Y = [ nodePositions( pointNumbers, : )'; 1 1 1 1 ];
    #     extraImageToImageTransformMatrix = Y * inv( originalY );
    #     scaling = svd( extraImageToImageTransformMatrix( 1 : 3, 1 : 3 ) );
    #     disp( [ 'scaling: ' num2str( scaling' ) ] )
    #   end
    #
    # end % End loop over iterations
    # numberOfIterations
    number_of_iterations = 0
    maximal_defomation = True
    while maximal_defomation:
        number_of_iterations += 1
        min_log_likelihood_times_prior, maximal_defomation = optimizer.step_optimizer()
        logger.info("at step %d maximal_defomation=%f log prior=%f", number_of_iterations, maximal_defomation, min_log_likelihood_times_prior)
        if show_figures:
            show_optimization_figures()
    return number_of_iterations


def show_optimization_figures():
    #   if showFigures
    #     % Show figure
    #     priors = kvlRasterizeAtlasMesh( mesh, size( imageBuffer ) );
    #     colorCodedPriors = kvlColorCodeProbabilityImages( priors, colors );
    #     mask = ( sum( double( priors ), 4 ) / (2^16-1) ) > .5;
    #     subplot( 2, 2, 1 )
    #     showImage( imageBuffer );
    #     subplot( 2, 2, 2 )
    #     showImage( imageBuffer .* mask );
    #     subplot( 2, 2, 3 )
    #     imageToShow = ( imageBuffer - min( imageBuffer(:) ) ) / ( max( imageBuffer(:) ) - min( imageBuffer(:) ) );
    #     imageToShow = ( 1 - priorVisualizationAlpha ) * repmat( imageToShow, [ 1 1 1 3 ] ) + ...
    #                   priorVisualizationAlpha * colorCodedPriors;
    #     showImage( imageToShow )
    #     subplot( 2, 2, 4 )
    #     tmp = double( priors( :, :, :, gmClassNumber ) ) / ( 2^16-1 );
    #     showImage( mosaicImages( tmp, imageBuffer .* mask, 2 ) );
    #     drawnow
    #
    #     % Show affine matrix, retrieved from any four non-colinear points before and after registration
    #     nodePositions = kvlGetMeshNodePositions( mesh );
    #     pointNumbers = [ 1 111 202 303 ];
    #     originalY = [ originalNodePositions( pointNumbers, : )'; 1 1 1 1 ];
    #     Y = [ nodePositions( pointNumbers, : )'; 1 1 1 1 ];
    #     extraImageToImageTransformMatrix = Y * inv( originalY );
    #     scaling = svd( extraImageToImageTransformMatrix( 1 : 3, 1 : 3 ) );
    #     disp( [ 'scaling: ' num2str( scaling' ) ] )
    #   end
    pass


def save_results(
        down_sampling_factors,
        image_buffer,
        image_file_name,
        initial_image_to_image_transform_matrix,
        mesh,
        original_node_positions,
        save_path,
        template_file_name
):
    #
    # % Retrieve the implicitly applied affine matrix from any four non-colinear points before and after registration,
    # % taking into account the downsampling that we applied
    # nodePositions = kvlGetMeshNodePositions( mesh );
    # pointNumbers = [ 1 111 202 303 ];
    # originalY = [ diag( downSamplingFactors ) * originalNodePositions( pointNumbers, : )'; 1 1 1 1 ];
    # Y = [ diag( downSamplingFactors ) * nodePositions( pointNumbers, : )'; 1 1 1 1 ];
    # extraImageToImageTransformMatrix = Y * inv( originalY );
    #
    # % Final result: the image-to-image (from template to image) as well as the world-to-world transform that
    # % we computed (the latter would be the identity matrix if we didn't move the image at all)
    # imageToImageTransformMatrix = extraImageToImageTransformMatrix * initialImageToImageTransformMatrix;
    # worldToWorldTransformMatrix = imageToWorldTransformMatrix * imageToImageTransformMatrix * ...
    #                               inv( templateImageToWorldTransformMatrix );
    world_to_world_transform_matrix = None
    # [ ~, templateFileNameBase, templateFileNameExtension ] = fileparts( templateFileName );
    # transformationMatricesFileName = fullfile( savePath, ...
    #                                            [ templateFileNameBase '_coregistrationMatrices.mat' ] );
    # eval( [ 'save ' transformationMatricesFileName ' imageToImageTransformMatrix worldToWorldTransformMatrix;' ] )
    #
    # % Compute the talairach.xfm
    # % Load fsaverage orig.mgz -- this is the ultimate target/destination
    # fshome = getenv('FREESURFER_HOME');
    # fnamedst = sprintf('%s/subjects/fsaverage/mri/orig.mgz',fshome);
    # fsaorig = MRIread(fnamedst,1);
    # % Compute the vox2vox from the template to fsaverage assuming they
    # %   share world RAS space
    # RAS2LPS = diag([-1 -1 1 1]);
    # M = inv(RAS2LPS*fsaorig.vox2ras)*(templateImageToWorldTransformMatrix);
    # % Compute the input to fsaverage vox2vox by combining the
    # % input-template vox2vox and the template-fsaverage vox2vox
    # X = M*inv(imageToImageTransformMatrix);
    # % Now write out the LTA. This can be used as the talairach.lta in recon-all
    # invol = MRIread(imageFileName,1); % have to reread to get header info
    # lta.type = 0;
    # lta.xform = X;
    # lta.srcfile = imageFileName;
    # lta.srcmri = invol;
    # lta.srcmri.vol = [];
    # lta.dstfile = fnamedst;
    # lta.dstmri = fsaorig;
    # lta.dstmri.vol = [];
    # lta.subject = 'fsaverage';
    # ltaFileName = sprintf('%s/samseg.talairach.lta',savePath);
    # lta_write(ltaFileName,lta);
    # fprintf('Done computng and writing out LTA %s\n',ltaFileName);
    #
    # % For historical reasons, we applied the estimated transformation to the template; let's do that now
    # desiredTemplateImageToWorldTransformMatrix = imageToWorldTransformMatrix * imageToImageTransformMatrix
    # transformedTemplateFileName = fullfile( savePath, ...
    #                                         [ templateFileNameBase '_coregistered' templateFileNameExtension ] );
    # kvlWriteImage( template, transformedTemplateFileName, ...
    #                kvlCreateTransform( desiredTemplateImageToWorldTransformMatrix ) );
    #
    #
    # % For debugging and/or quality control purposes, save a picture of the registration result to file
    # priors = kvlRasterizeAtlasMesh( mesh, size( imageBuffer ) );
    # colorCodedPriors = kvlColorCodeProbabilityImages( priors, colors );
    # mask = ( sum( double( priors ), 4 ) / (2^16-1) ) > .5;
    # overlayQcImage = ( imageBuffer - min( imageBuffer(:) ) ) / ( max( imageBuffer(:) ) - min( imageBuffer(:) ) );
    # overlayQcImage = ( 1 - priorVisualizationAlpha ) * repmat( overlayQcImage, [ 1 1 1 3 ] ) + ...
    #               priorVisualizationAlpha * colorCodedPriors;
    #
    # tmp = double( priors( :, :, :, gmClassNumber ) ) / ( 2^16-1 );
    # mosaicQcImage = mosaicImages( tmp, imageBuffer .* mask, 2 );
    #
    # overlayCollage = getCollage( overlayQcImage, 10 );
    # mosaicCollage = getCollage( mosaicQcImage, 10 );
    #
    # borderSize = 20;
    # DIM = [ size( overlayCollage, 1 ) size( overlayCollage, 2 ) ];
    # qcFigure = zeros( [ DIM( 1 )  2 * DIM( 2 ) 3 ]  + [ 2*borderSize 3*borderSize 0 ] ) + .5;
    # qcFigure( borderSize + [ 1 : DIM( 1 ) ], borderSize + [ 1 : DIM( 2 ) ], : ) = overlayCollage;
    # qcFigure( borderSize + [ 1 : DIM( 1 ) ], ...
    #           2 * borderSize + DIM( 2 ) + [ 1 : DIM( 2 ) ], : ) = mosaicCollage;
    # qcFigureFileName = fullfile( savePath, ...
    #                              [ templateFileNameBase '_coregistrationCqFigure.png' ] );
    # imwrite( qcFigure, qcFigureFileName )
    #
    return world_to_world_transform_matrix
