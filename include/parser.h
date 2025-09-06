#pragma once
#include <stddef.h>

typedef enum {
  ROOT = 0,
  HTML,
  HEAD,
  TITLE,
  BODY,
  COND,   // 属性（bgcolor, font, href など）
  ANCH,   // aタグ
  BR,
  TEXT
} tag_t;

/* 簡易DOMノード */
typedef struct node {
  tag_t tag;
  char  content[256];        // 固定長（安全にstrncpyでコピー）
  struct node *parent;
  struct node *child;
  struct node *next;
} node_t;

/* ルート初期化 */
int     init_tree(node_t *root);

/* HTMLを走査してDOMを構築（depthはログ用） */
void    find_tag(const char *src, int depth);

/* デバッグ出力 */
void    show_tree(node_t *root);

/* 最初に見つかった指定タグのcontentを返す（存在しなければ空文字列） */
char*   solve_node(node_t *root, tag_t tag);

/* 最初に見つかった指定タグのノードを返す（なければNULL） */
node_t* solve_body(node_t *root, tag_t tag);

/* 子ノードを末尾に追加して返す（3引数に統一） */
node_t* add_node(int tag, const char *content, node_t *parent);
