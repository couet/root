#ifndef TMVA_SOFIE_ROPERATOR_RESHAPE
#define TMVA_SOFIE_ROPERATOR_RESHAPE

#include "TMVA/SOFIE_common.hxx"
#include "TMVA/ROperator.hxx"
#include "TMVA/RModel.hxx"

#include <cassert>
#include <sstream>

namespace TMVA{
namespace Experimental{
namespace SOFIE{

enum ReshapeOpMode { Reshape, Flatten, Squeeze, Unsqueeze };


class ROperator_Reshape final : public ROperator
{

private:

   bool fVerbose = false;
   ReshapeOpMode fOpMode = Reshape;   // type of Reshape operator

   int fAllowZero = 0; // (for Reshape) zero in tensor shape makes output shape equal to input tensor shape
   int fAxis = 1;      // (for Flatten)

   std::string fNData;        // input data tensor name
   std::string fNShape;       // reshape tensor name
   std::string fNOutput;               // output tensor name
   std::vector<size_t> fShapeInput;     // input shape data
   std::vector<size_t> fShapeOutput;   // output shape data
   std::vector<int64_t> fAttrAxes;         // axes attributes (provided for all version of Squeeze/Unsqueeze)

public:

   std::string Name() const {
      if (fOpMode == Reshape) return "Reshape";
      if (fOpMode == Flatten) return "Flatten";
      if (fOpMode == Squeeze) return "Squeeze";
      if (fOpMode == Unsqueeze) return "Unsqueeze";
      return "";
   }

   ROperator_Reshape(){}
   ROperator_Reshape(ReshapeOpMode opMode, int attr_value, std::string nameData, std::string nameShape, std::string nameOutput)
      : fOpMode(opMode), fNData(UTILITY::Clean_name(nameData)), fNShape(UTILITY::Clean_name(nameShape)),
      fNOutput(UTILITY::Clean_name(nameOutput))
   {
      if (opMode == Reshape) fAllowZero = attr_value;
      if (opMode == Flatten) fAxis = attr_value;

      fInputTensorNames = { fNData };
      if(!fNShape.empty()){
         fInputTensorNames.emplace_back(fNShape);
      }
      fOutputTensorNames = { fNOutput };
   }

   // for squeeze/unsqueezed operators following old ONNX version (< 10)
   // In this cases axes are passed as attribute values
   ROperator_Reshape(ReshapeOpMode opMode, std::vector<int64_t> attrAxes, std::string nameData, std::string nameOutput)
      : fOpMode(opMode), fNData(UTILITY::Clean_name(nameData)), fNOutput(UTILITY::Clean_name(nameOutput)),
        fAttrAxes(attrAxes)
   {
      assert(fOpMode == Squeeze || fOpMode == Unsqueeze);
   }

   // output type is same as input
   std::vector<ETensorType> TypeInference(std::vector<ETensorType> input) override {
      auto ret = std::vector<ETensorType>(1, input[0]);
      return ret;
   }

   // output shape
   std::vector<std::vector<size_t>> ShapeInference(std::vector<std::vector<size_t>> input) override {
      std::vector<std::vector<size_t>> ret;
      auto & input_shape = input[0];

      if (fOpMode == Reshape) {
         if (input.size() != 2) throw std::runtime_error("TMVA SOFIE Reshape Op needs 2 input tensors");
         auto output_shape = input[1]; // the provided shape
         size_t input_length = ConvertShapeToLength(input_shape);
         size_t output_length = ConvertShapeToLength(output_shape);
         // (input_length == output_length) is the easy case : (2,3,4) -> (2,12)
         if (input_length != output_length) {
            if ((output_length == 0 && fAllowZero == 0) || static_cast<long>(output_length)  < 0) {
               // in this case value 0 or -1 in shape are automatically corrected
               bool replacementDone = false;
               for (size_t i = 0; i < output_shape.size(); i++) {
                  if (output_shape[i] == 0 || output_shape[i] == static_cast<size_t>(-1)) {
                     if (replacementDone) {
                        throw std::runtime_error("TMVA Reshape Op : output shape has multiple negative or zero values");
                     }
                     auto tmp = output_shape;
                     tmp.erase(tmp.begin() + i);
                     auto tmp_length = ConvertShapeToLength(tmp);
                     output_shape[i] = input_length / tmp_length;
                     replacementDone = true;
                  }
               }
               if (fVerbose)
                  std::cout << "Reshape: correct output shape from " << ConvertShapeToString(input[1])
                        << " to " << ConvertShapeToString(output_shape) << std::endl;
            }
            if (ConvertShapeToLength(output_shape) != input_length) {
               throw std::runtime_error("TMVA Reshape Op : Invalid  shapes : " + ConvertShapeToString(input_shape) +
                                        ConvertShapeToString(output_shape));
            }
         }
         ret.push_back(output_shape);

      } else if (fOpMode == Flatten) {
         // flattenig case
         size_t inputSize = ConvertShapeToLength(input_shape);
         size_t b = input[0][0];
         std::vector<size_t> newShape = {b, inputSize / b};
         ret.push_back(newShape);

      } else if (fOpMode == Squeeze) {
         // squeeze
         // assume no axis is provided - remove all axes with value equal to 1
         auto output_shape = input[0];
         if (input.size() == 1) {
            size_t i = 0;
            while (i < output_shape.size()) {
               if (output_shape[i] == 1 ) {
                  output_shape.erase(output_shape.begin() + i);
               }
               else {
                  i++;
               }
            }
         } else if (input.size() == 2) {
            auto & axes = input[1];
            for (size_t i = 0; i < axes.size(); i++){
               if (output_shape[axes[i]] != 1)
                  throw std::runtime_error("TMVA Squeeze Op : Invalid  axes : " + ConvertShapeToString(axes) +
                                           ConvertShapeToString(output_shape));
               output_shape.erase(output_shape.begin() + axes[i]);
            }
         }
         ret.push_back(output_shape);
      }

      else if (fOpMode == Unsqueeze) {
         // unsqueeze
         assert(input.size() == 2);
         auto output_shape = input[0];
         auto &axes = input[1];
         // output rank
         int64_t r = input[0].size() + axes.size();
         for (auto & a : axes) {
            int64_t i = static_cast<int64_t>(a);
            if ( i < -r  || i > r - 1 )
               throw std::runtime_error("TMVA Unsqueeze Op - axes input is not in correct range");
            if (i >= 0)
               output_shape.insert(output_shape.begin() + i, 1);
            else
               //negative axes
               output_shape.insert(output_shape.end() + i + 1, 1);
         }
         ret.push_back(output_shape);
      }
      return ret;
   }

   void Initialize(RModel& model) override {

      fVerbose = model.Verbose();
      if (model.CheckIfTensorAlreadyExist(fNData) == false) {
          // input must be a graph input, or already initialized intermediate tensor
         throw std::runtime_error("TMVA Reshape Op Input Tensor " + fNData + "  is not found in model");
      }
      fShapeInput = model.GetTensorShape(fNData);
      // check if optional shape tensor exist
      if (!fNShape.empty()) {
         if (model.CheckIfTensorAlreadyExist(fNShape)) {
            auto dptr = model.GetInitializedTensorData(fNShape);
            auto input_shape = static_cast<int64_t *>(dptr.get());
            auto vec = model.GetTensorShape(fNShape);
            assert(vec.size() == 1);
            size_t n = vec[0]; // size of shape input tensor

            std::vector<size_t> descShape(n);
            std::copy(input_shape, input_shape + n, descShape.begin());
            fShapeOutput = ShapeInference({fShapeInput, descShape})[0];
            // set flag to not write tensor in weight file. Its data will be hard-coded in way model is constructed
            model.SetNotWritableInitializedTensor(fNShape);
         } else {
            throw std::runtime_error("TMVA Reshape Op Shape Tensor " + fNShape + " is not found in model");
         }
      } else if (!fAttrAxes.empty()) {
         // case fNShape is empty and axes are provided as attributes
         std::vector<size_t> descShape(fAttrAxes.size());
         std::copy(fAttrAxes.begin(), fAttrAxes.end(), descShape.begin());
         fShapeOutput = ShapeInference({fShapeInput, descShape})[0];
      } else if (fOpMode == Flatten || fOpMode == Squeeze) {
         fShapeOutput = ShapeInference({fShapeInput})[0];
      } else {
         throw std::runtime_error("TMVA Reshape Op : Invalid Input/Attribute data");
      }
      // check if output is constant or not
      if (model.IsInitializedTensor(fNData) && model.GetTensorType(fNData) == ETensorType::INT64) {
         fIsOutputConstant = true;
         auto inputData = static_cast<int64_t*>(model.GetInitializedTensorData(fNData).get());
         if (ConvertShapeToLength(fShapeInput) != ConvertShapeToLength(fShapeOutput))
            throw std::runtime_error("TMVA Reshape Op : Invalid Input/Output lengths");
         model.AddConstantTensor<int64_t>(fNOutput, fShapeOutput, inputData);
         if (model.Verbose()) {
            std::cout << Name() << " : " << fNData << " " << ConvertShapeToString(fShapeInput) << " -->  " << fNOutput << " (constant) " << ConvertShapeToString(fShapeOutput)  << " : " <<
            ConvertValuesToString(ConvertShapeToLength(fShapeOutput), inputData) << std::endl;
         }
      } else {
         // non-constant case
         model.AddIntermediateTensor(fNOutput, model.GetTensorType(fNData), fShapeOutput);
         if (model.Verbose())
            std::cout << Name() << " : " << fNData << " " << ConvertShapeToString(fShapeInput) << " -->  "<< fNOutput << "  " << ConvertShapeToString(fShapeOutput)  << std::endl;
      }
   }

   std::string Generate(std::string OpName) override {
      if (fIsOutputConstant) return "";  //no op for constant tensors

      OpName = "op_" + OpName;

      // output of reshape is same as input
      size_t length = ConvertShapeToLength(fShapeOutput);
      if (length != ConvertShapeToLength(fShapeInput)) {
         throw std::runtime_error("TMVA SOFIE Reshape Op : wrong output shape - is " +
                                  ConvertShapeToString(fShapeOutput) + " and input is " +
                                  ConvertShapeToString(fShapeInput));
      }
      std::stringstream out;
      std::string opName = "Reshape";
      if (fOpMode == Flatten)
         opName = "Flatten";
      else if (fOpMode == Squeeze)
         opName = "Squeeze";
      else if (fOpMode == Unsqueeze)
         opName = "Unsquueze";

      out << SP << "///--------" << opName << " operator\n" << std::endl;
      out << SP << "std::copy( tensor_" << fNData << ", tensor_" << fNData << " + " << length << ", " << "tensor_" << fNOutput
          << ");\n";
      return out.str();
   }
};

}//SOFIE
}//Experimental
}//TMVA


#endif //TMVA_SOFIE_ROPERATOR_RESHAPE
