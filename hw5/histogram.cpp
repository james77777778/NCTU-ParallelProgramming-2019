#include <stdio.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <ios>
#include <time.h>
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif

typedef struct
{
    uint8_t R;
    uint8_t G;
    uint8_t B;
    uint8_t align;
} RGB;

typedef struct
{
    bool type;
    uint32_t size;
    uint32_t height;
    uint32_t weight;
    RGB *data;
    uint8_t *pixel;
} Image;

Image *readbmp(const char *filename)
{
    FILE *bmp;
    bmp = fopen(filename, "rb");
    // std::ifstream bmp(filename, std::ios::binary);
    char header[54];
    // bmp.read(header, 54);
    fread(header, sizeof(char), 54, bmp);
    uint32_t size = *(int *)&header[2];
    uint32_t offset = *(int *)&header[10];
    uint32_t w = *(int *)&header[18];
    uint32_t h = *(int *)&header[22];
    uint16_t depth = *(uint16_t *)&header[28];
    if (depth != 24 && depth != 32)
    {
        printf("we don't suppot depth with %d\n", depth);
        exit(0);
    }
    fseek(bmp, offset, SEEK_SET);
    // bmp.seekg(offset, bmp.beg);

    Image *ret = new Image();
    ret->type = 1;
    ret->height = h;
    ret->weight = w;
    ret->size = w * h;
    ret->pixel = new uint8_t[w * h * 3]{};
    fread(ret->pixel, sizeof(uint8_t), ret->size*3, bmp);
    fclose(bmp);
    return ret;
}

int writebmp(const char *filename, Image *img)
{

    uint8_t header[54] = {
        0x42,        // identity : B
        0x4d,        // identity : M
        0, 0, 0, 0,  // file size
        0, 0,        // reserved1
        0, 0,        // reserved2
        54, 0, 0, 0, // RGB data offset
        40, 0, 0, 0, // struct BITMAPINFOHEADER size
        0, 0, 0, 0,  // bmp width
        0, 0, 0, 0,  // bmp height
        1, 0,        // planes
        32, 0,       // bit per pixel
        0, 0, 0, 0,  // compression
        0, 0, 0, 0,  // data size
        0, 0, 0, 0,  // h resolution
        0, 0, 0, 0,  // v resolution
        0, 0, 0, 0,  // used colors
        0, 0, 0, 0   // important colors
    };

    // file size
    uint32_t file_size = img->size * 4 + 54;
    header[2] = (unsigned char)(file_size & 0x000000ff);
    header[3] = (file_size >> 8) & 0x000000ff;
    header[4] = (file_size >> 16) & 0x000000ff;
    header[5] = (file_size >> 24) & 0x000000ff;

    // width
    uint32_t width = img->weight;
    header[18] = width & 0x000000ff;
    header[19] = (width >> 8) & 0x000000ff;
    header[20] = (width >> 16) & 0x000000ff;
    header[21] = (width >> 24) & 0x000000ff;

    // height
    uint32_t height = img->height;
    header[22] = height & 0x000000ff;
    header[23] = (height >> 8) & 0x000000ff;
    header[24] = (height >> 16) & 0x000000ff;
    header[25] = (height >> 24) & 0x000000ff;

    FILE *fout;
    fout = fopen(filename, "wb");
    fwrite(header, sizeof(char), 54, fout);
    fwrite(img->data, sizeof(char), img->size * 4, fout);
    fclose(fout);
}

cl_program load_program(cl_context context, const char* filename)
{
    std::ifstream in(filename, std::ios_base::binary);
    if(!in.good()) {
        return 0;
    }
    // get file length
    in.seekg(0, std::ios_base::end);
    size_t length = in.tellg();
    in.seekg(0, std::ios_base::beg);

    // read program source
    std::vector<char> data(length + 1);
    in.read(&data[0], length);
    data[length] = 0;

    // create and build program 
    const char* source = &data[0];
    cl_program program = clCreateProgramWithSource(context, 1, &source, 0, 0);
    if(program == 0) {
        return 0;
    }
    if(clBuildProgram(program, 0, 0, 0, 0, 0) != CL_SUCCESS) {
        return 0;
    }
    return program;
}

int main(int argc, char *argv[])
{
    char *filename;
    if (argc >= 2)
    {
        int many_img = argc - 1;
        // cl init
        cl_int cl_err, cl_err2, cl_err3, cl_err4; // return value of CL functions, check whether OpenCL platform errors
        cl_uint num_device, num_plat;
        cl_device_id device_id;
        cl_platform_id plat_id;
        cl_context ctx;
        cl_command_queue que;
        // num_entries = 1 for 1 GPU card
        cl_err = clGetPlatformIDs(1, &plat_id, &num_plat); // get platform id and number
        cl_err2 = clGetDeviceIDs(plat_id, CL_DEVICE_TYPE_GPU, 1, &device_id,  &num_device);
        cl_context_properties prop[] = {CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(plat_id), 0};
        ctx = clCreateContextFromType(prop, CL_DEVICE_TYPE_DEFAULT, NULL, NULL, &cl_err3);
        que = clCreateCommandQueue(ctx, device_id, 0, &cl_err4);

        if (cl_err == CL_SUCCESS && cl_err2 == CL_SUCCESS && cl_err3 == CL_SUCCESS && cl_err4 == CL_SUCCESS) {
            // printf("CL platform init OK \n");
        }
        else {
            printf("CL platform init failed \n");
            return 1;
        }

        cl_program program = load_program(ctx, "histogram.cl");
        cl_int cl_err_kernel;
        cl_kernel histogram = clCreateKernel(program, "histogram", &cl_err_kernel);

        // create RGB
        uint32_t R[256];
        uint32_t G[256];
        uint32_t B[256];

        for (int i = 0; i < many_img; i++)
        {
            filename = argv[i + 1];
            Image *img = readbmp(filename);

            std::cout << img->weight << ":" << img->height << "\n";

            int data_size = img->weight*img->height;
            cl_mem cl_pixel = clCreateBuffer(ctx, CL_MEM_READ_ONLY, sizeof(uint8_t) * data_size * 3, NULL, NULL);
            clEnqueueWriteBuffer(que, cl_pixel, CL_FALSE, 0, sizeof(uint8_t) * data_size * 3, img->pixel, 0, NULL, NULL);
            std::fill(R, R+256, 0);
            std::fill(G, G+256, 0);
            std::fill(B, B+256, 0);
            cl_mem cl_R = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, sizeof(uint32_t) * 256, NULL, NULL);
            cl_mem cl_G = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, sizeof(uint32_t) * 256, NULL, NULL);
            cl_mem cl_B = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, sizeof(uint32_t) * 256, NULL, NULL);
            clEnqueueWriteBuffer(que, cl_R, CL_FALSE, 0, sizeof(uint32_t) * 256, R, 0, NULL, NULL);
            clEnqueueWriteBuffer(que, cl_G, CL_FALSE, 0, sizeof(uint32_t) * 256, G, 0, NULL, NULL);
            clEnqueueWriteBuffer(que, cl_B, CL_FALSE, 0, sizeof(uint32_t) * 256, B, 0, NULL, NULL);
            clFlush(que);
            clFinish(que);
            clSetKernelArg(histogram, 0, sizeof(cl_mem), &cl_pixel);
            clSetKernelArg(histogram, 1, sizeof(cl_mem), &cl_R);
            clSetKernelArg(histogram, 2, sizeof(cl_mem), &cl_G);
            clSetKernelArg(histogram, 3, sizeof(cl_mem), &cl_B);

            size_t work_size = data_size*3;
            cl_int err = clEnqueueNDRangeKernel(que, histogram, 1, 0, &work_size, 0, 0, 0, 0);

            if(err == CL_SUCCESS) {
                err = clEnqueueReadBuffer(que, cl_R, CL_FALSE, 0, sizeof(uint32_t) * 256, R, 0, 0, 0);
                err = clEnqueueReadBuffer(que, cl_G, CL_FALSE, 0, sizeof(uint32_t) * 256, G, 0, 0, 0);
                err = clEnqueueReadBuffer(que, cl_B, CL_FALSE, 0, sizeof(uint32_t) * 256, B, 0, 0, 0);
                clFlush(que);
                clFinish(que);
            }
            else{
                std::cerr << "Can't run kernel: "<< err << "\n";
            }

            if(err == CL_SUCCESS) {
            }
            else {
                std::cerr << "Can't read back data: " << err << "\n";
            }
            int max = 0;
            for(int i=0;i<256;i++){
                max = R[i] > max ? R[i] : max;
                max = G[i] > max ? G[i] : max;
                max = B[i] > max ? B[i] : max;
            }

            Image *ret = new Image();
            ret->type = 1;
            ret->height = 256;
            ret->weight = 256;
            ret->size = 256 * 256;
            ret->data = new RGB[256 * 256];

            for(int i=0;i<ret->height;i++){
                for(int j=0;j<256;j++){
                    if(R[j]*256/max > i)
                        ret->data[256*i+j].R = 255;
                    if(G[j]*256/max > i)
                        ret->data[256*i+j].G = 255;
                    if(B[j]*256/max > i)
                        ret->data[256*i+j].B = 255;
                }
            }
            std::string newfile = "hist_" + std::string(filename); 
            writebmp(newfile.c_str(), ret);
        }
    // release
    clFlush(que);
    clFinish(que);
    clReleaseKernel(histogram);
    clReleaseProgram(program);
    clReleaseCommandQueue(que);
    clReleaseContext(ctx);
    }else{
        printf("Usage: ./hist <img.bmp> [img2.bmp ...]\n");
    }
    return 0;
}
