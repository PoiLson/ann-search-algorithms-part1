import struct

def extract_first_n_fvecs(input_file, output_file, n=100):
    count = 0
    with open(input_file, "rb") as fin, open(output_file, "wb") as fout:
        while count < n:
            dim_bytes = fin.read(4)
            if not dim_bytes:
                break
            dim = struct.unpack("i", dim_bytes)[0]
            vector_bytes = fin.read(4 * dim)
            if len(vector_bytes) < 4 * dim:
                break
            fout.write(dim_bytes)
            fout.write(vector_bytes)
            count += 1
    print(f"Wrote first {count} vectors to {output_file}")

extract_first_n_fvecs("Data/SIFT/sift_query.fvecs", "Data/SIFT/sift_query_100.fvecs", 100)
