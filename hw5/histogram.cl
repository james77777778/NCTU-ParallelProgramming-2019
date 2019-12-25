__kernel void histogram(__global uchar *img_data, __global unsigned int *R, __global unsigned int *G, __global unsigned int *B, const int stride)
{
    int idx = get_global_id(0);
    int pos = (idx % stride);
    unsigned int rgb = img_data[idx] & 0xFF;
    if (pos == 0)
        atomic_inc(&R[rgb]);
    else if (pos == 1)
        atomic_inc(&G[rgb]);
    else if (pos == 2)
        atomic_inc(&B[rgb]);
}