#include <mutex>
#include <random>
#include <unordered_map>

#include <libhat/System.hpp>

int main(){
	const auto& system = hat::get_system();
	
	printf("cpu_vendor: %s\n", system.cpu_vendor.c_str());
	printf("cpu_brand: %s\n", system.cpu_brand.c_str());
	// extensions
	const auto& ext = system.extensions;
	printf("sse: %d\n", ext.sse);
	printf("sse2: %d\n", ext.sse2);
	printf("sse3: %d\n", ext.sse3);
	printf("ssse3: %d\n", ext.ssse3);
	printf("sse41: %d\n", ext.sse41);
	printf("sse42: %d\n", ext.sse42);
	printf("avx: %d\n", ext.avx);
	printf("avx2: %d\n", ext.avx2);
	printf("avx512f: %d\n", ext.avx512f);
	printf("avx512bw: %d\n", ext.avx512bw);
	printf("popcnt: %d\n", ext.popcnt);
	printf("bmi: %d\n", ext.bmi);

	return 0;
}