## 1、Perf工具使用
### 1.1 常用指令
(1) `perf stat`
- 作用：统计程序运行的核心性能指标（CPU 周期、指令数、缓存命中率等），快速定位是否有性能问题
- 用法：
```shell
# 分析 ls 命令的性能（基础用法)
sudo perf stat ls 
# 分析你自己的程序（比如你的 CUDA/C 程序） 
sudo perf stat ./your_cuda_program
```
- 输出解析：
以下图为例，逐个解释每个显示项的含义：
`task-clock`：程序占用的CPU时间。
`context-switches`：运行中没有发生进程上下文切换 → 程序全程独占 CPU，没有被其他进程打断。
`cpu-migrations`：没有发生CPU核心迁移 → 程序一直在同一个核心上执行。
`page-faults`：发生了缺页中断的次数。
`cycles`：总共消耗了多少个CPU时钟周期。
`instructions`：执行的总指令数。
`branches`：总共执行多少次分支指令。
`branch-misses`：分支预测失败。
![[截屏2026-03-14 23.55.00.png]]

(2) `perf top`
- 作用：实时监控系统进程的CPU占用，按函数/模块排序，快速找到最耗时的代码段。
- 用法：
```shell
# 全局监控（看整个系统） 
sudo perf top 
# 只监控某个进程（pid 换成你的程序进程号） 
sudo perf top -p <pid>
```

(3) `perf record\report`
- 作用：记录程序运行的所有性能数据，事后生成报告，精准定位到具体函数 / 代码行的瓶颈。
- 用法：
```shell
# 第一步：记录程序运行数据（-g 记录函数调用栈） 
sudo perf record -g ./your_program 
# 第二步：生成可视化报告 
sudo perf report
```
- 输出解析：
执行后会生成如下性能图，效果类似执行top指令，按照列，逐个解释每列的数据的含义。
`Children`：该函数及其所有子函数总共占用的CPU周期百分比。
`Self`：该函数自身代码占用的CPU周期百分比。
`Command`：触发该函数的进程名。
`Shared Object`：该函数来自的二进制文件。
`Symbol`：函数名
![[截屏2026-03-15 00.50.21.png]]
上图主要显示的是当前系统中所有函数执行的性能占比，如果想要看具体的函数，可以进入到对应的函数中，这边以进入isort函数为例，会显示以下几个选项：
`Annotate isort`：把 `isort` 函数拆解到**汇编指令级别**，并标注每一行代码 / 指令的 CPU 耗时占比。
`Zoom into isort thread`：只显示`isort`所在线程的性能数据，过滤掉其他线程的干扰。
`Zoom into isort DSO`：只显示来自`isort`这个二进制文件的所有函数，过滤掉`libc`、内核等其他模块的函数。
`Expand [isort] callchain`：展开`isort`的调用栈，看它被谁调用、又调用了谁。
`Browse map details`：查看`isort`函数在内存中的地址映射、段信息。
`Run scripts for samples of symbol [isort]`：对 `isort` 函数的采样数据运行自定义脚本（比如统计、导出数据等）。
`Run scripts for all samples`：对整个 `perf.data` 里的所有采样数据运行自定义脚本。
`Switch to another data file in PWD`：切换到当前目录下其他 `perf.data` 文件（比如你多次 `perf record` 生成了不同数据文件）。
`Exit`：关闭菜单。
![[截屏2026-03-15 19.11.49.png]]

(4) `perf trace`
- 作用：监控程序调用的所有系统调用（比如 read/write/socket），看是否有 IO / 网络瓶颈。
- 用法：
```shell
sudo perf trace ./your_program
```

(5) `perf mem`
- 作用：专门分析内存/缓存的访问效率。
- 用法：
```shell
sudo perf mem record ./your_program 
sudo perf mem report
```

## 2、Perf实战
作业2要求对isort进行性能瓶颈定位，接下来，将通过两步来找到isort中的性能瓶颈，作业提供的isort核心算法源码如下：
```C
/* Insertion sort */
void isort(data_t* left, data_t* right) {
	data_t* cur = left + 1;
	while (cur <= right) {
		data_t val = *cur;
		data_t* index = cur - 1;
	
		while (index >= left && *index > val) {
			*(index + 1) = *index;
			index--;
		}
		*(index + 1) = val;
		cur++;
	}
}
```
### 2.1 初步定位
拿到程序后，首先进行初判，判断程序是否存在明显的性能瓶颈，执行`sudo perf stat ./isort 10000 10`获得如下数据：
![[Pasted image 20260315192459.png]]
#### 2.1.1 基础运行特征
- `task-clock：233.01msec`：程序实际占用CPU时间为0.233秒。
- `CPUs utilized：0.999`：单线程的情况下跑满单个核心，合理。
- `context-switches：0/cpu-migrations：0`：不存在上下文切换，说明没有中断进来，抢走CPU。
- `page-faults：70/sys time：0`：出现70次缺页中断，而实际内核态执行的时间是0，说明这70次缺页中断全都是出现在程序的初始化阶段，在程序的实际运行过程中并没有出现缺页中断，因此可以判断缺页中断也在合理范围内。
#### 2.1.2 CPU指令与流水线效率
判断CPU流水线效率最核心的数据是`IPC(insn per cycle)`，也就是每秒执行多少条指令。isort这个程序一个周期能够执行5.83条指令，属于效率拉满。
#### 2.1.3 分支预测表现
从图中可以看到该程序，分支预测错误的比率只占`0.01%`，所以显然这里也不是性能瓶颈。
#### 2.1.4 小结
从上述数据可以看出，在机器的执行效率上，已经没有可以进一步优化的地方，因此该程序真正的瓶颈就在算法上。

#### 2.2 算法性能瓶颈定位
从算法这个角度去定位性能瓶颈，其实就是看看算法中，哪一行代码的执行成本最高，然后看看能不能通过优化，将整体执行的指令数下降，同时还需要保证CPU执行效率等宏观性能特征不要变糟糕。
执行指令`sudo perf annotate --stdio -s isort`获得性能数据，并将结果显示在终端上，核心耗时程序段如下图所示：
![[截屏2026-03-15 20.33.31.png|400]]
通过上述结果可以直接定位到整个程序，在代码行`while (index >= left && *index > val)`耗时总和最高，将近58%的时间都用在了这里，那么主要原因是什么呢？要知道原因，就得读得懂while循环这行代码中实际的汇编指令做了什么：
```asm
# -------------------------- 第一步：判断 index >= left -------------------------- 11cb: mov -0x28(%rbp),%rcx # 读 index（指针）到 rcx 寄存器 | 0.56% 
11cf: xor %eax,%eax # eax 清零（关键指令）| 10.60% 
11d1: cmp -0x8(%rbp),%rcx # 比较 rcx(index) 和 -0x8(%rbp)(left) | 0.85% 
11d5: mov %al,-0x29(%rbp) # 把比较结果（al）写入栈内存 | 0.28% 
11d8: jb 11ed <isort+0x5d> # index < left → 跳转到 11ed | 2.73% 
# -------------------------- 第二步：判断 *index > val -------------------------- 11de: mov -0x28(%rbp),%rax # 读 index 到 rax 寄存器 | 0.66% 
11e2: mov (%rax),%eax # 读 *index 的值到 eax | 11.28% 
11e4: cmp -0x1c(%rbp),%eax # 比较 *index 和 -0x1c(%rbp)(val) | 4.22% 
11e7: seta %al # *index > val → al=1，否则 al=0 | 1.60% 
11ea: mov %al,-0x29(%rbp) # 把 al 写入栈内存 -0x29(%rbp) | 13.72% 
# -------------------------- 第三步：判断 && 结果 -------------------------- 
11ed: mov -0x29(%rbp),%al # 读栈内存的结果到 al | 17.42% 
11f0: test $0x1,%al # 测试 al 是否为 1（即 && 结果为真） | 5.07% 
11f2: jne 11fd <isort+0x6d> # 结果为真 → 进入循环体 | 0.00% 
11f8: jmp 121b <isort+0x8b> # 结果为假 → 跳出循环 | 0.00%
```
耗时指令：
- `11cf: xor %eax,%eax # eax 清零`
这里`%eax`寄存器的主要作用是存储`index >= left`的比较结果，clang编译器为了避免原始的脏数据干扰比较结果，因此对这个寄存器进行了提前清零，但是这个行为是**冗余操作**，因为旧数据其实是会被直接覆盖，由于这是个计算指令，且每次循环都一定会执行，所以导致耗时占比较高。
- `11e2: mov (%rax),%eax # 读 *index 的值到 eax`
这行汇编指令主要是对指针`index`进行解引用，然后读取index的值，所以每次读取指针`index`指向的值都需要从缓存或者内存中读取，比读写寄存器要慢得多，因此耗时占比比较高。
-  `11ea: mov %al,-0x29(%rbp) # 把 al 写入栈内存 -0x29(%rbp)`
这行的作用主要是把`*index > val`的比较结果写入栈内存，**其实这里感觉比较奇怪，就是汇编指令11d5和这行的功能一模一样，为什么耗时差这么多，在执行次数上，11d5应该每次循环都会执行，但是11ea还有可能会被直接跳过**
- `11ed: mov -0x29(%rbp),%al # 读栈内存的结果到 al`
这块也不太懂为什么。

## 3、Cachegrind
### 3.1 什么是Cachegrind
Cachegrind 是 Valgrind 工具集里的**缓存和分支预测分析工具**，核心作用是**精准统计程序执行过程中发生的缓存命中 / 缺失、分支预测命中 / 失败次数**，帮你找到程序中因「缓存效率低」「分支预测差」导致的性能瓶颈。

### 3.2 相关指令
`valgrind --tool=cachegrind --branch-sim=yes <program_name> <program_arguments>`

### 3.3 常用缩写含义速查
- `Ir`：指令读取次数
- `I1mr`：`L1`指令缓存缺失次数
- `ILmr`：末级指令缓存缺失次数
- `Dr`：数据读取次数
- `D1mr`：`L1`数据缓存缺失次数
- `DLmr`：末级数据缓存缺失次数
- `Dw`：数据写入次数
- `D1mw`：`L1`数据写入缺失次数
- `DLmw`：末级数据写入缺失次数

### 3.4  `cg_annotate`工具
`cg_annotate`工具主要是用来辅助`cachegrind`，它可以查看函数级甚至汇编级的缓存性能数据。
#### 3.4.1 Options
- `--pid`
    Indicates which `cachegrind.out.pid` file to read. Not actually an option -- it is required.

- `-h, --help`
    `-v, --version`
    Help and version, as usual.

- `--sort=A,B,C` [default: order in `cachegrind.out.pid`]
    Specifies the events upon which the sorting of the function-by-function entries will be based. Useful if you want to concentrate on eg. I cache misses (`--sort=I1mr,I2mr`), or D cache misses (`--sort=D1mr,D2mr`), or L2 misses (`--sort=D2mr,I2mr`).

- `--show=A,B,C` [default: all, using order in `cachegrind.out.pid`]
    Specifies which events to show (and the column order). Default is to use all present in the `cachegrind.out.pid` file (and use the order in the file).

- `--threshold=X` [default: 99%]
    Sets the threshold for the function-by-function summary. Functions are shown that account for more than X% of the primary sort event. If auto-annotating, also affects which files are annotated.
    Note: thresholds can be set for more than one of the events by appending any events for the `--sort` option with a colon and a number (no spaces, though). E.g. if you want to see the functions that cover 99% of L2 read misses and 99% of L2 write misses, use this option:
    `--sort=D2mr:99,D2mw:99`

- `--auto=no` [default]
    `--auto=yes`
    When enabled, automatically annotates every file that is mentioned in the function-by-function summary that can be found. Also gives a list of those that couldn't be found.

- `--context=N` [default: 8]
    Print N lines of context before and after each annotated line. Avoids printing large sections of source files that were not executed. Use a large number (eg. 10,000) to show all source lines.

- `-I<dir>, --include=<dir>` [default: empty string]
    Adds a directory to the list in which to search for files. Multiple -I/--include options can be given to add multiple directories.
### 3.4 `sum`文件缓存命中分析
#### 3.4.1 任务目标
使用cachegrind分析sum.c程序的缓存性能，并尝试使用sum.c中的值N和U来减少缓存未命中的数量。

#### 3.4.2 sum.c程序代码
```C
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef uint32_t data_t;
const int U = 100000; // size of the array. 10 million vals ~= 40MB
const int N = 100000; // number of searches to perform

int main() {
	data_t* data = (data_t*) malloc(U * sizeof(data_t));
	if (data == NULL) {
		free(data);
		printf("Error: not enough memory\n");
		exit(-1);
	}

	// fill up the array with sequential (sorted) values.
	int i;
	for (i = 0; i < U; i++) {
		data[i] = i;
	}

	printf("Allocated array of size %d\n", U);
	printf("Summing %d random values...\n", N);

	data_t val = 0;
	data_t seed = 42;
	for (i = 0; i < N; i++) {
		int l = rand_r(&seed) % U;
		val = (val + data[l]);
	}
	free(data);
	printf("Done. Value = %d\n", val);
	return 0;
}
```
#### 3.4.3 查看CPU各级缓存行信息
(1) 查看CPU缓存大小
命令：`lscpu`
输出：
![[截屏2026-03-18 21.54.15.png]]

(2) 查看CPU缓存行大小
命令：`getconf LEVEL2_DCACHE_LINESIZE`
输出：`64`
#### 3.4.4 Cache性能剖析
(1) 程序整体Cache性能剖析
指令：`valgrind --tool=cachegrind --branch-sim=yes ./sum
输出结果：
```plaintxt
U = 100000, N = 100000
/* 指令缓存部分 */
==4820== I refs: 4,838,950 # 总指令访问次数 
==4820== I1 misses: 1,351 # L1 指令缓存缺失次数 
==4820== LLi misses: 1,349 # L2/L3 末级指令缓存缺失次数 
==4820== I1 miss rate: 0.03% # L1 指令缓存缺失率 (1351 / 4,838,950) 
==4820== LLi miss rate: 0.03% # 末级指令缓存缺失率 (1349 / 4,838,950)

/* 数据缓存部分 */
==4820== D refs: 2,147,234 (1,433,341 rd + 713,893 wr) # 总数据访问次数（读+写） ==4820== D1 misses: 96,358 (89,423 rd + 6,935 wr) # L1 数据缓存缺失次数 
==4820== LLd misses: 8,144 (1,303 rd + 6,841 wr) # 末级数据缓存缺失次数 
==4820== D1 miss rate: 4.5% (6.2% + 1.0%) # L1 数据缓存缺失率 
==4820== LLd miss rate: 0.4% (0.1% + 1.0%) # 末级数据缓存缺失率

/* 末级缓存整体 */
==4820== LL refs: 97,709 (90,774 rd + 6,935 wr) # 访问末级缓存的总次数（即 L1 缺失后触发的查询） 
==4820== LL misses: 9,493 (2,652 rd + 6,841 wr) # 末级缓存缺失次数（真正访问内存） ==4820== LL miss rate: 0.1% (0.0% + 1.0%) # 末级缓存缺失率

/* 分支预测部分 */
==4820== Branches: 323,603 (223,223 cond + 100,380 ind) # 总分支数（条件分支+间接分支） 
==4820== Mispredicts: 3,070 (2,897 cond + 173 ind) # 预测错误次数 
==4820== Mispred rate: 0.9% (1.3% + 0.2%) # 分支预测错误率
```

(2) 函数级缓存性能分析
指令：`cg_annotate cachegrind.out.xx > cg_report.txt`
结果中可以看到有几处缓存失效较高的地方，首先是第一处地方，写指令缓存失效的次数占90%，分析一下这块地方是否合理，这块主要是一个for循环，然后往data数组中写数据。

失效次数合理性分析：数组内部储存的元素的大小是4字节，循环次数是10000次，缓存行大小是64字节，因此理论上的写缓存失效次数是：`ceil(400,000 ÷ 64) = 6,250`，与实际值相等，因此合理。

二级缓存和一级缓存失效次数相等分析：
写缓存失效的过程如下，首先写入某个缓存行的时候，L1数据缓存中没有该行，从而触发L1缓存写失效；然后CPU会先加载整个缓存行；而L1缓存必然缺失，因此向上查询L2，由于该数组是首次访问，所以L2缓存中也没有该数据，会发生缓存失效，因此每加载1行，就会发生一次L1的缓存失效和一次L2级的缓存失效。
![[截屏2026-03-21 22.53.14.png]]

第二处地方：
第二处地方在语句`val = (val + data[l]);`发生了98%的一级读缓存失效，原因非常好理解，因为这块是随机读取数组中的值，必然会发生非常多次的读缓存失效。
![[截屏2026-03-22 14.43.14.png]]
要消除这里的读缓存，我们只需要把N设置成16以内的值，就能保证，data每次取到的值都是同一缓存行内的，结果如下所示：
![[截屏2026-03-22 15.53.19.png]]

### 3.5 Homework：Sorting
#### 3.5.1 Write-up 1
```
Write-up 1: Compare the Cachegrind output on the DEBUG=1 code versus DEBUG=0 compiler
optimized code. Explain the advantages and disadvantages of using instruction count as a
substitute for time when you compare the performance of different versions of this program.
```
- `DEBUG = 0; N = 10000; R = 50`
![[截屏2026-03-22 17.28.00.png]]

- `DEBUG = 1; N = 10000; R = 50`
![[截屏2026-03-22 17.29.34.png]]
现象：开启DEBUG的版本，不管是指令数还是数据访问次数都更多，但是指令失效次数两个版本接近。
用指令数代替运行时间的优缺点：
- 优点：指令数能够直接反馈编译器/代码优化的效果，不受缓存命中、分支预测等“硬件特性”影响，能够单纯衡量"代码逻辑的复杂度"。
- 缺点：缓存失效次数、指令类型都会影响程序性能，单单从指令数来衡量性能，角度过于单一。
#### 3.5.2 Write-up 2
```
Write-up 2: Explain which functions you chose to inline and report the performance
differences you observed between the inlined and uninlined sorting routines.
```
在完成这个Write-up的时候，感觉使用clang的效果并不是特别好，一个是添加inline前后的预期时间差距很小，另一个是对递归函数的支持并不太好，所以在这个Write-up，我全部使用GCC来作为编译器。
编译命令：`gcc main.c tests.c util.c isort.c sort_a.c sort_c.c sort_i.c sort_p.c sort_m.c sort_f.c -O3 -DNDEBUG -g -Wall -std=gnu99 -gdwarf-3 --param max-inline-insns-recursive=450 -lrt -lm  -o sort`
- 原始文件
![[截屏2026-03-25 21.38.00.png]]

- 第一次尝试：`merge_i、copy_i设置为inline`
![[截屏2026-03-25 21.47.16.png]]

- 第二次尝试：`merge_i、copy_i、mem_alloc、mem_free设置为inline`
![[截屏2026-03-25 21.49.07.png]]

- 第三次尝试：`merge_i、copy_i、mem_alloc、mem_free、sort_i设置为inline`
![[截屏2026-03-25 21.51.43.png]]

- 第四次尝试：对递归函数的内联，会导致指令数快速膨胀，gcc默认情况下要求内联展开后的指令上限是450，尝试调大指令上限，从而增加递归函数展开的层数，可以通过添加编译选项：`--param max-inline-insns-recursive=2000`
![[截屏2026-03-25 21.52.46.png]]

- 第五次尝试：`mem_alloc、mem_free、sort_i设置为inline`
![[截屏2026-03-25 22.34.18.png]]

- 第六次尝试：`copy_i、mem_alloc、mem_free、sort_i设置为inline`
![[截屏2026-03-25 22.37.57.png]]
- 分析
每次改动，都使用perf stat来生成对应的perf数据，在perf生成的数据中，仅仅关注`instructions`和`time`这两个数据，分析以原始文件的数据为基线。
1、第一次将`merge_i`和`sort_i`设置为inline，指令数和时间均没有太大变化，由此怀疑GCC编译器在编译优化级别为O3时，会自动将`merge_i`和`sort_i`进行内联处理，后续通过查看汇编，也验证了这点。
2、第二次将`mem_alloc`和`mem_free`设置为inline，指令数减少了11.6%，时间减少了2%，数据证明这两个函数的内联是符合预期的。
3、第三次将`sort_i`设置为inline，指令数减少了12.6%，但是时间和第二次优化接近，也是2%左右。
4、第四次增加`sort_i`内联展开的层数，指令数减少了14.6%，时间减少了3.2%。
5、第五次尝试，指令数减少了10%，时间减少了3.2%。
6、第六次尝试，指令数减少了10%，但是时间减少了5%。

结论：
(1) 在目前的尝试中，第六次的优化尝试，时间减少的最多，但是指令却并不是最少的，由此可见，并不是内联的程度越高，性能越好，内联的程度越高，会导致可执行文件的指令数膨胀，进而增加指令缓存的失效次数，最终导致性能反而会有损失。
(2) inline关键字，只是程序员给编译器的建议，编译器并不一定会采用，如果需要强制某个函数内联化，可以使用`__attribute__((always_inline))`关键字。
(3) 每次优化尝试，都需要确认编译器按照我们的想法进行了内联，不然就会出现第一次优化尝试一样的结果，由于我对编译器的行为并不熟悉，所以我借助了[[https://godbolt.org]]该网站来帮助验证。


