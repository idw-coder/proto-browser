// src/parser.c
#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

extern node_t dom;

/* 3引数 add_node：ノード確保→親に接続 */
node_t* add_node(int tag, const char* content, node_t* parent)
{
  node_t *node = (node_t*)calloc(1, sizeof(*node));
  if (!node) return NULL;

  node->tag = (tag_t)tag;
  if (content) {
    size_t max = sizeof(node->content) - 1;
    strncpy(node->content, content, max);
    node->content[max] = '\0';
  } else {
    node->content[0] = '\0';
  }

  node->parent = parent;
  node->child  = NULL;
  node->next   = NULL;

  if (parent) {
    if (!parent->child) parent->child = node;
    else {
      node_t *last = parent->child;
      while (last->next) last = last->next;
      last->next = node;
    }
  }
  return node;
}

int init_tree(node_t* root)
{
  memset(root, 0, sizeof(*root));
  root->tag = ROOT;
  return 0;
}

/* 文字列トリム */
static void trim(char *s)
{
  if (!s) return;
  size_t i=0, len=strlen(s);
  while (i<len && (s[i]==' '||s[i]=='\t'||s[i]=='\n'||s[i]=='\r')) i++;
  if (i) memmove(s, s+i, len-i+1);
  len = strlen(s);
  while (len>0 && (s[len-1]==' '||s[len-1]=='\t'||s[len-1]=='\n'||s[len-1]=='\r')) s[--len]='\0';
}

/* name="val" を name/val に分解 */
static void divide_cond(const char *attr, char *name_out, size_t name_cap,
                        char *val_out,  size_t val_cap)
{
  if (!attr) { if(name_out) name_out[0]='\0'; if(val_out) val_out[0]='\0'; return; }
  const char *eq = strchr(attr, '=');
  if (!eq) {
    strncpy(name_out, attr, name_cap-1); name_out[name_cap-1]='\0'; trim(name_out);
    if (val_out) val_out[0]='\0';
    return;
  }
  size_t nlen = (size_t)(eq - attr); if (nlen >= name_cap) nlen = name_cap-1;
  memcpy(name_out, attr, nlen); name_out[nlen]='\0'; trim(name_out);

  const char *v = eq+1; while (*v==' '||*v=='\t') v++;
  if (*v=='"' || *v=='\'') {
    char q=*v++; size_t i=0;
    while (*v && *v!=q && i+1<val_cap) val_out[i++]=*v++;
    val_out[i]='\0';
  } else {
    strncpy(val_out, v, val_cap-1); val_out[val_cap-1]='\0';
  }
  trim(val_out);
}

/* …（show_tree / solve_node / solve_body はあなたの元コードで可） */

/* 小文字化 */
static void lower_token(const char *p, char *out, size_t cap)
{
  size_t i=0;
  while (*p && *p!='>' && *p!=' ' && *p!='\t' && *p!='\n' && i+1<cap) {
    char c=*p++;
    if ('A'<=c && c<='Z') c = (char)(c-'A'+'a');
    out[i++]=c;
  }
  out[i]='\0';
}

/* テキストを吐き出す C関数（ラムダ禁止なので外出し） */
static void flush_text(node_t *parent, char *buf)
{
  trim(buf);
  if (buf[0]) { add_node(TEXT, buf, parent); buf[0]='\0'; }
}

/* メインのパース。旧4引数 add_node 呼び出しを全て3引数に変更 */
void find_tag(const char *src, int depth)
{
  (void)depth; if (!src) return;

  node_t *root  = &dom;
  node_t *n_html= add_node(HTML, NULL, root);
  node_t *n_head= add_node(HEAD, NULL, n_html);
  node_t *n_body= add_node(BODY, NULL, n_html);

  /* <title>…</title> */
  const char *t1 = strcasestr(src, "<title>");
  const char *t2 = strcasestr(src, "</title>");
  if (t1 && t2 && t2>t1) {
    t1 += 7;
    size_t len = (size_t)(t2 - t1);
    char buf[256]; if (len >= sizeof(buf)) len = sizeof(buf)-1;
    memcpy(buf, t1, len); buf[len]='\0'; trim(buf);
    add_node(TITLE, buf, n_head);
  }

  /* <body ...> 属性 → COND(name) の子に TEXT(value) をぶら下げる */
  const char *b1 = strcasestr(src, "<body");
  if (b1) {
    const char *b2 = strchr(b1, '>');
    if (b2 && b2>b1) {
      char attr[256]; size_t alen=(size_t)(b2-(b1+5));
      if (alen >= sizeof(attr)) alen = sizeof(attr)-1;
      memcpy(attr, b1+5, alen); attr[alen]='\0';

      char *p = attr;
      while (*p) {
        while (*p==' '||*p=='\t'||*p=='\n'||*p=='\r') p++;
        if (!*p) break;
        char token[128]=""; size_t ti=0;
        while (*p && !(*p==' '||*p=='\t'||*p=='\n'||*p=='\r')) {
          if (ti+1<sizeof(token)) token[ti++]=*p;
          p++;
        }
        token[ti]='\0';
        if (token[0]) {
          char name[64], val[128];
          divide_cond(token, name, sizeof(name), val, sizeof(val));
          if (name[0] && val[0]) {
            node_t *c = add_node(COND, name, n_body);
            add_node(TEXT, val, c);
          }
        }
      }
    }
  }

  /* 本文抽出（<body>..</body> 範囲） */
  const char *body_s = strcasestr(src, "<body");
  const char *body_e = strcasestr(src, "</body>");
  if (!body_s) body_s = src;
  else { const char *gt=strchr(body_s,'>'); if (gt) body_s = gt+1; }
  if (!body_e) body_e = src + strlen(src);

  const char *p = body_s;
  char textbuf[256] = "";

  while (p < body_e && *p) {
    if (*p == '<') {
      flush_text(n_body, textbuf);     // ← ラムダの代わり
      const char *q = strchr(p, '>');
      if (!q) break;
      char name[32]; lower_token(p+1, name, sizeof(name));
      if (strcmp(name,"br")==0) {
        add_node(BR, NULL, n_body);
      } else if (strcmp(name,"a")==0) {
        const char *gt = q;
        const char *href = strcasestr(p, "href=");
        char hrefv[128] = "";
        if (href && href < gt) {
          href += 5; while (*href==' '||*href=='\t') href++;
          if (*href=='"' || *href=='\'') {
            char quote=*href++; size_t i=0;
            while (*href && *href!=quote && i+1<sizeof(hrefv)) hrefv[i++]=*href++;
            hrefv[i]='\0';
          }
        }
        const char *a_end = strcasestr(q+1, "</a>");
        char linktext[256] = "";
        if (a_end) {
          size_t len=(size_t)(a_end-(q+1));
          if (len >= sizeof(linktext)) len = sizeof(linktext)-1;
          memcpy(linktext, q+1, len); linktext[len]='\0'; trim(linktext);
        }
        node_t *a = add_node(ANCH, NULL, n_body);
        if (hrefv[0]) { node_t *c = add_node(COND, "href", a); add_node(TEXT, hrefv, c); }
        if (linktext[0]) add_node(TEXT, linktext, a);

        p = a_end ? a_end+4 : q+1;
        continue;
      }
      p = q + 1;
    } else {
      size_t len = strlen(textbuf);
      if (len+1 < sizeof(textbuf)) textbuf[len] = *p, textbuf[len+1]='\0';
      p++;
    }
  }
  flush_text(n_body, textbuf);         // ← 最後に吐く
}
void show_tree(node_t *root)
{
  if (!root) return;
  static const char* tname[]={"ROOT","html","head","title","body","cond","a","br","text"};
  node_t *st[1024]; int top=0;
  if (root->child) st[top++]=root->child;
  while (top>0) {
    node_t *n = st[--top];
    if (!n) continue;
    printf("[%s] %s\n", tname[n->tag], n->content);
    if (n->next) st[top++]=n->next;
    if (n->child) st[top++]=n->child;
  }
}

char* solve_node(node_t *root, tag_t tag)
{
  static char empty[] = "";
  if (!root) return empty;
  node_t *st[1024]; int top=0;
  if (root->child) st[top++]=root->child;
  while (top>0) {
    node_t *n = st[--top];
    if (!n) continue;
    if (n->tag == tag) return n->content;
    if (n->next) st[top++]=n->next;
    if (n->child) st[top++]=n->child;
  }
  return empty;
}

node_t* solve_body(node_t *root, tag_t tag)
{
  if (!root) return NULL;
  node_t *st[1024]; int top=0;
  if (root->child) st[top++]=root->child;
  while (top>0) {
    node_t *n = st[--top];
    if (!n) continue;
    if (n->tag == tag) return n;
    if (n->next) st[top++]=n->next;
    if (n->child) st[top++]=n->child;
  }
  return NULL;
}