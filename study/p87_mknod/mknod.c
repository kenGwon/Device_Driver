#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h> 
#include <unistd.h> 
#include <stdio.h>

int main(int argc, char *argv[])
{
	int ret;

	ret = mknod("/dev/devfile", S_IRWXU | S_IRWXG | S_IFCHR, (240<<8) | 1); // 상위 8비트는 주번호로, 하위 8비트는 부번호로 쓴다.

	if (ret < 0)
	{
		perror("mknod()");
		return -ret; // 시스템 콜의 에러코드는 음수로 나온다. 그런데 bash shell에서 볼 수 있는 결과 코드는 양수여야 제대로 보인다.(음수면 보수를 취해주면서 값이 이상하게 보인다.) 그래서 한번더 -를 붙여서 양수로 바꿔준다.
	}

	return 0;
}
