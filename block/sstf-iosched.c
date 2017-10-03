/*
 * SSTF IO Scheduler - Implemented by Carlos Moratelli
 *
 * For Kernel 4.12.4
 * 
 */
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>

int cur = 0;

/* SSTF data structure. */
struct sstf_data {
	struct list_head queue;
};

/* Esta função despacha o próximo bloco a ser lido. */
static int sstf_dispatch(struct request_queue *q, int force){
	struct sstf_data *nd = q->elevator->elevator_data;
	char direction = 'R';
	struct request *rq, *best;
    int cur = 0;
	/* Aqui deve-se retirar uma requisição da fila e enviá-la para processamento.
	 * Use como exemplo o driver noop-iosched.c. Veja como a requisição é tratada.
	 * 
	 * Antes de retornar da função, imprima o sector que foi atendido.
	 */
	best = list_entry(&nd->queue, struct request, queuelist);
	
	if (list_empty(&nd->queue)) return 0;
	
    list_for_each_entry(rq, &nd->queue, queuelist)
    {
        if (abs(cur - blk_rq_pos(rq)) < abs(cur - blk_rq_pos(best)))
        {
            best = rq;
        }
    }
    

	printk(KERN_EMERG "[SSTF] dsp %c %lu\n", direction, blk_rq_pos(best));

    if (best)
    {
        /*
        cur = blk_rq_pos(best);
		list_del_init(&best->queuelist);
		elv_dispatch_sort(q, best);
		*/
		list_del_init(&best->queuelist);
		elv_dispatch_sort(q, best);
		cur = blk_rq_pos(best);
		return 1;
	}
	return 0;
}

static void sstf_add_request(struct request_queue *q, struct request *rq){
	struct sstf_data *nd = q->elevator->elevator_data;
	char direction = 'R';

	/* Aqui deve-se adicionar uma requisição na fila do driver.
	 * Use como exemplo o driver noop-iosched.c
	 * 
	 * Antes de retornar da função, imprima o sector que foi adicionado na lista.
	 */

	list_add_tail(&rq->queuelist, &nd->queue);	
	printk(KERN_EMERG "[SSTF] add %c %lu\n", direction, blk_rq_pos(rq));
}


static int sstf_init_queue(struct request_queue *q, struct elevator_type *e){
	struct sstf_data *nd;
	struct elevator_queue *eq;

	eq = elevator_alloc(q, e);
	if (!eq)
		return -ENOMEM;

	nd = kmalloc_node(sizeof(*nd), GFP_KERNEL, q->node);
	if (!nd) {
		kobject_put(&eq->kobj);
		return -ENOMEM;
	}
	eq->elevator_data = nd;

	INIT_LIST_HEAD(&nd->queue);

	spin_lock_irq(q->queue_lock);
	q->elevator = eq;
	spin_unlock_irq(q->queue_lock);
	
	return 0;
}

static void sstf_exit_queue(struct elevator_queue *e)
{
	struct sstf_data *nd = e->elevator_data;

    BUG_ON(!list_empty(&nd->queue));
	kfree(nd);
}

/* Infrastrutura dos drivers de IO Scheduling. */
static struct elevator_type elevator_sstf = {
	.ops.sq = {
		.elevator_dispatch_fn		= sstf_dispatch,
		.elevator_add_req_fn		= sstf_add_request,
		.elevator_init_fn		= sstf_init_queue,
		.elevator_exit_fn		= sstf_exit_queue,
	},
	.elevator_name = "sstf",
	.elevator_owner = THIS_MODULE,
};

/* Inicialização do driver. */
static int __init sstf_init(void)
{
	elv_register(&elevator_sstf);

	return 0;
}

/* Finalização do driver. */
static void __exit sstf_exit(void)
{
	elv_unregister(&elevator_sstf);
}

module_init(sstf_init);
module_exit(sstf_exit);


MODULE_AUTHOR("Carlos Moratelli");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SSTF IO scheduler");
