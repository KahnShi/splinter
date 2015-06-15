classdef PSpline < Spline
    properties (Access = protected)
        Handle
        
        Constructor_function = 'pspline_init';
        Destructor_function = 'pspline_delete';
        Eval_function = 'pspline_eval';
        EvalJacobian_function = 'pspline_eval_jacobian';
        EvalHessian_function = 'pspline_eval_hessian';
        Save_function = 'pspline_save';
        Load_function = 'pspline_load';
    end

    methods
        % Constructor. Creates an instance of the PSpline class in the
        % library, using the samples in dataTable.
        % lambda is the smoothing parameter, usually a small number
        % default: 0.03
        function obj = PSpline(dataTable, lambda)
            if(~exist('lambda', 'var'))
                lambda = 0.03;
            end
            fprintf('%d\', lambda)
            % Set to -1 so we don't try to delete the library instance in case type is invalid
            obj.Handle = -1;
            
            obj.Handle = calllib(obj.Splinter_alias, obj.Constructor_function, dataTable.get_handle(), lambda);

            if(obj.Handle == 0)
                error('Could not create BSpline!');
            end
        end
    end
end
