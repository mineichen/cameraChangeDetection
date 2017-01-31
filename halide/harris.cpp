#include <halide/Halide.h>
#include <halide/halide_image_io.h>
#include <vector>
#include <cassert>


#include <stdio.h>

using namespace Halide;

/**
 * Helper function which setup a new function with the sum of neighbour pixels multiplied
 * with the vector-entries. Therefore, the Vector has to have a uneven number of elements.
 * The element in the middle will be multiplied with the actual cell
 *
 * This method reads underliing data around vector.size() times
 */
Func neighbourX(Func input, std::vector<int8_t> kernel) {
    Func f;
    Var x, y;
    
    int kernelSize = kernel.size();
    int halfKernelSize = kernelSize / 2;
    assert(kernelSize % 2 == 1);
    
    Expr exp = 0;
    
    for(int i = 0; i < kernelSize; i++) {
        int distance = i - halfKernelSize;
        int8_t nextWeight = kernel[i];
        
        exp = exp + (input(x + distance, y) * nextWeight);
    }
    f(x,y) = exp;
    
    return f;
}

/**
 * Same as neighbourX, but for y direction
 */
Func neighbourY(Func input, std::vector<int8_t> kernel) {
    Func f;
    Var x, y;
    
    int kernelSize = kernel.size();
    int halfKernelSize = kernelSize / 2;
    assert(kernelSize % 2 == 1);
    
    Expr exp = 0;
    
    for(int i = 0; i < kernelSize; i++) {
        int distance = i - halfKernelSize;
        int8_t nextWeight = kernel[i];
        
        exp = exp + (input(x, y + distance) * nextWeight);
    }
    f(x,y) = exp;
    
    return f;
}


Func neighbour(Func input, std::vector<int8_t> kernelX, std::vector<int8_t> kernelY)
{
    return neighbourY(
        neighbourX(input, kernelX),
        kernelY
    );
}

Func neighbour(Func input, std::vector<int8_t> kernel)
{
    return neighbour(input, kernel, kernel);
}

/**
 * Transforms a input image into a grayscale Image
 */
Image<uint8_t> grayScale(Image<float> in) {
    Image<uint8_t> out(in.width(), in.height());
    Func rgb2gray;
    Var x,y;
    
    rgb2gray(x, y) = cast<uint8_t>(
       (0.3f *in(x,y,0)
       +0.59f*in(x,y,1)
       +0.11f*in(x,y,2)) * 255
    );
    rgb2gray.realize(out);
    
    return out;
}

class HarrisGenerator : public Generator<HarrisGenerator> {
public:
    ImageParam input{UInt(8), 2, "input"};
    
    enum class OutputType { HarrisImage, BlockMaxMatrix };
    GeneratorParam<OutputType> outputType{"outputType",
        /* default value */
        OutputType::HarrisImage,
        /* map from names to values */
        {{ "HarrisImage", OutputType::HarrisImage },
         { "BlockMaxMatrix", OutputType::BlockMaxMatrix }}};
    
    /**
     * Ignored if OutputType != BlockMaxMatrix
     *
     * Should be a divisor of inputWidth - 2, because the input image is blurred first
     * Because these widths-2 usually don't have many divisors, consider to use just a subset of the input image
     */
    GeneratorParam<uint8_t> blockMaxWidth{"blockMaxWidth", 8, 0, 255};
    
    /**
     * See blockMaxWidth
     */
    GeneratorParam<uint8_t> blockMaxHeight{"blockMaxHeight", 8, 0, 255}; // Should be a divisor of inputHeight -2
    
    GeneratorParam<bool> protectOverflow{"protectOverflow", true};
    GeneratorParam<int> outputShift{"outputShift", 0, 0, 63};
    GeneratorParam<Halide::Type> outputDatetype{"outputDatetype", Int(64)};
    
    Var x, y;
    int kernelOffset;
    
    // We then define a method that constructs and return the Halide
    // pipeline:
    Func build() {
        std::vector<int8_t> sobelEdges = {1,2,3,4,3,2,1};
        
        kernelOffset = 1 + (sobelEdges.size() - 1)/2;
        
        // If we want to inspect just one of the terms, we can wrap
        // 'print' around it like so:
        Func
            F("F"),
            X("X"),
            Y("Y"),
            X2("X2"),
            Y2("Y2"),
            XY("XY"),
            X2B("X2B"),
            Y2B("Y2B"),
            XY2("XY2"),
            XY2B("XY2B"),
            Corner("Corner");
        
        F(x,y) = cast<int16_t>(input(x,y)); // Output 0-255
        X(x,y) = cast<uint16_t>(Halide::abs(neighbour(F, {1,2,1}, {-1,0,1})(x,y)) >> 2); //Output 0-255
        Y(x,y) = cast<uint16_t>(Halide::abs(neighbour(F, {-1,0,1}, {1,2,1})(x,y)) >> 2); //Output 0-255
        
        X2(x,y) = (X(x,y) * X(x,y)) >> 8;
        Y2(x,y) = (Y(x,y) * Y(x,y)) >> 8;
        XY(x,y) = (X(x,y) * Y(x,y)) >> 8; //abs above doesn't make a difference because XY is never used after square in next row, where it becomes positive anyway
        XY2(x,y) = cast<int64_t>(XY(x,y)) * XY(x,y);
        
        
        X2.compute_root()
            .vectorize(x, 4)
            .compute_at(X2B, x);
        
        Y2.compute_at(Y2B, y);
        
        XY2.compute_root()
        //  .compute_at(XY2B, x)
        ;
        
        int mult =  std::accumulate(sobelEdges.begin(), sobelEdges.end(), 0);
        X2B(x,y) = neighbour(X2, sobelEdges)(x,y) / mult;
        Y2B(x,y) = neighbour(Y2, sobelEdges)(x,y) / mult;
        XY2B(x,y) = neighbour(XY2, sobelEdges)(x,y) / mult;
        // All Values are are in between 0 and 255
        
        X2B.compute_root();
        Y2B.compute_root();
        XY2B.compute_root();
        
        
        Expr X2PlusY2B = X2B(x,y) + Y2B(x,y);
        Expr cornerExp = cast<int64_t>((X2B(x,y) * Y2B(x,y)) - XY2B(x,y) - cast<int64_t>(0.04f * (X2PlusY2B * X2PlusY2B)));
        
        if (outputShift > 0) {
            cornerExp = cornerExp >>(uint8_t) outputShift;
        }
        
        switch (outputType) {
            case OutputType::HarrisImage:
                if(protectOverflow) {
                    Corner(x,y) = cast(outputDatetype, Halide::min(255, Halide::max(0, cornerExp >> (uint8_t) outputShift)));
                } else {
                    Corner(x,y) = cast(outputDatetype, cornerExp);
                }
                break;
            //case OutputType::BlockMaxMatrix:
                //@todo Reduce image here like: maxValue, maxX, maxY
            //  Corner(x,y) = cornerExp;
                
                
                //Var x_inner("x_inner"), x_outer("x_outer"), y_inner("y_inner"), y_outer("y_outer");
                //Func t, subsample("subsample");
                //t(x,y) = cornerExp;
                //t.tile(x,y,x_outer,y_outer,x_inner,y_inner, 2, 2);
                //
                /*
                //Expr curValue = tiled(x_outer + box.x, y_outer + box.y);
                //Expr condition = maximum(clamped(x + box.x, y + box.y))
                RDom r(0, 2, 0, 2);
                subsample(x, y) = t(2 * x + r, 2 * y + r);
                
                Corner(x,y) = Tuple(0, 0, subsample(0,0));
                Corner(x,y) = tuple_select(
                    subsample(r.x, r.y) > Corner()[2],
                    Tuple(r.x, r.y, subsample(r.x, r.y)),
                    Corner()
                );
                                     
                                     
                
                
                Corner(x_outer, y_outer) = argmax(x_outer + r, subsample);
                
                Corner() = {select(Corner()[1] > subsample(r.x, r.y), r, Corner()[0]);}
                
                break;
                 */
            default:
                throw std::runtime_error("Unknown outputType");
        }
                                       
        return Corner;
    }
};
RegisterGenerator<HarrisGenerator> harris_generator{"harris_generator"};

int main(int argc, char **argv)
{
#if 1
    return Halide::Internal::generate_filter_main(argc, argv, std::cerr);
#else
    /**
     * c++ -g harris.cpp -std=c++11 -lHalide `libpng-config --cflags --ldflags` -O3 -fno-rtti -o build/harris_generator && ./build/harris_generator
     */
    
    Image<uint8_t> in = grayScale(Tools::load_image("../data/room.png"));
    
    HarrisGenerator gen;
    gen.input.set(in);
    gen.outputShift.set(2);
    gen.outputDatetype.set(UInt(8));
    
    Func a = gen.build();
    Image<uint8_t> cornerImg(in.width()-2*gen.kernelOffset, in.height()-2*gen.kernelOffset);
    cornerImg.set_min(gen.kernelOffset,gen.kernelOffset);
    a.realize(cornerImg);
        
    Tools::save_image(cornerImg, "build/bright.png");
    
    return 0;
#endif
}

