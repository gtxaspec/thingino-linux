#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/slab.h>

#if defined (CONFIG_COMMON_CLK_NA51055) || defined (CONFIG_COMMON_CLK_NA51089)
void pll_dump_info(void);
#endif
#define MAX_CMD_LENGTH 30
#define MAX_ARG_NUM 6

static int clk_dump_info_cmd(struct seq_file *s, void *data)
{
#if defined (CONFIG_COMMON_CLK_NA51055) || defined (CONFIG_COMMON_CLK_NA51089)
	pll_dump_info();
#else
	seq_printf(s, "use [ echo @device > clk_dump_info ] to dump info\n");
#endif
	return 0;
}

static int nvt_proc_clk_dump_info_cmd(struct inode *inode, struct file *file)
{
	return single_open(file, clk_dump_info_cmd, NULL);
}

static const struct file_operations clk_dump_info_fops = {
	.open = nvt_proc_clk_dump_info_cmd,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

typedef struct proc_pinmux {
	struct proc_dir_entry *pproc_clk_info_dump_entry;
} proc_clk_t;
proc_clk_t proc_clk_dbg;

static int __init nvt_clk_dbg_proc_init(void)
{
	int ret = 0;
	struct proc_dir_entry *pentry = NULL;

	pentry = proc_create("nvt_info/nvt_clk/clk_dump_info", S_IRUGO | S_IXUGO, NULL, &clk_dump_info_fops);
	if (pentry == NULL) {
		pr_err("failed to create proc clk dump info!\n");
		ret = -EINVAL;
		goto remove_proc;
	}
	proc_clk_dbg.pproc_clk_info_dump_entry = pentry;

remove_proc:
	return ret;
}

late_initcall(nvt_clk_dbg_proc_init);
