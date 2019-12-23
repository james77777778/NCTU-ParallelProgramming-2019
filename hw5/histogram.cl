__kernel void histogram(__global unsigned int *img_data, __global unsigned int *R, __global unsigned int *G, __global unsigned int *B)
{
    int idx = get_global_id(0);
    int pos = (idx % 3);
    if (pos == 0)
        atomic_inc(&R[img_data[idx]]);
    else if (pos == 1)
        atomic_inc(&G[img_data[idx]]);
    else if (pos == 2)
        atomic_inc(&B[img_data[idx]]);
}