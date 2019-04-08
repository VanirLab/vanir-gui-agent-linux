struct genlist {
	long key;
	void *data;
	struct genlist *next;
	struct genlist *prev;
};

struct genlist *list_new(void);
struct genlist *list_lookup(struct genlist *l, long key);
struct genlist *list_insert(struct genlist *l, long key, void *data);
void list_remove(struct genlist *);
