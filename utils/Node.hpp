#pragma once

extern "C" {
#include <lexbor/css/css.h>
#include <lexbor/html/html.h>
#include <lexbor/selectors/selectors.h>
}

#include "Utils.hpp"
#include <iostream>
#include <re2/re2.h>
#include <regex>

class Node {
public:
  Node(const string &rawHtml) {
    // init document
    document = lxb_html_document_create();
    if (document == NULL) {
      throw;
    }

    unsigned char *html =
        reinterpret_cast<unsigned char *>(const_cast<char *>(rawHtml.c_str()));

    // parse html
    status = lxb_html_document_parse(document, html, rawHtml.size());
    if (status != LXB_STATUS_OK) {
      throw;
    }

    body = lxb_dom_interface_node(lxb_html_document_body_element(document));
    if (body == NULL) {
      throw;
    }

    // init css parser
    parser = lxb_css_parser_create();
    status = lxb_css_parser_init(parser, NULL);
    if (status != LXB_STATUS_OK) {
      throw;
    }

    // init selectors
    selectors = lxb_selectors_create();
    status = lxb_selectors_init(selectors);
    if (status != LXB_STATUS_OK) {
      throw;
    }
  }

  Node(lxb_dom_node_t *node) {
    body = node;

    // init css parser
    parser = lxb_css_parser_create();
    status = lxb_css_parser_init(parser, NULL);
    if (status != LXB_STATUS_OK) {
      throw;
    }

    // init selectors
    selectors = lxb_selectors_create();
    status = lxb_selectors_init(selectors);
    if (status != LXB_STATUS_OK) {
      throw;
    }
  }

  ~Node() {
    // Destroy Selectors object.
    (void)lxb_selectors_destroy(selectors, true);

    // Destroy resources for CSS Parser.
    (void)lxb_css_parser_destroy(parser, true);

    // Destroy all Selector List memory.
    lxb_css_selector_list_destroy_memory(list);

    // Destroy HTML Document.
    lxb_html_document_destroy(document);
  }

  Node *find(string selector) {
    vector<Node *> nodes = findAll(selector);

    if (nodes.size() > 1) {
      for (size_t i = 1; i < nodes.size(); ++i) {
        delete nodes[i];
      }
    }

    if (nodes.size() < 1)
      throw "Element not found";

    return nodes.at(0);
  }

  Node *tryFind(string selector) {
    try {
      return this->find(selector);
    } catch (...) {
      return nullptr;
    }
  }

  vector<Node *> findAll(string selector) {
    unsigned char *slctrs =
        reinterpret_cast<unsigned char *>(const_cast<char *>(selector.c_str()));

    list = lxb_css_selectors_parse(parser, slctrs, selector.size());
    if (parser->status != LXB_STATUS_OK) {
      throw;
    }

    vector<Node *> nodes;

    status = lxb_selectors_find(
        selectors, body, list,
        [](lxb_dom_node_t *node, lxb_css_selector_specificity_t spec,
           void *ctx) -> lxb_status_t {
          vector<Node *> *nodeVector = reinterpret_cast<vector<Node *> *>(ctx);
          nodeVector->push_back(new Node(node));

          return LXB_STATUS_OK;
        },
        reinterpret_cast<void *>(&nodes));

    if (status != LXB_STATUS_OK) {
      throw;
    }

    return nodes;
  }

  string rawContent() {
    string contentString;

    (void)lxb_html_serialize_deep_cb(
        body,
        [](const lxb_char_t *data, size_t len, void *ctx) -> lxb_status_t {
          string *content = reinterpret_cast<string *>(ctx);
          content->append(reinterpret_cast<const char *>(data), len);
          return LXB_STATUS_OK;
        },
        &contentString);

    return contentString;
  }

  string text() {
    string result = this->rawContent();
    RE2::GlobalReplace(&result, RE2("<[^>]*>"), "");
    return result;
  }

  string getAttribute(string attribute) {
    string tagString;

    (void)lxb_html_serialize_cb(
        body,
        [](const lxb_char_t *data, size_t len, void *ctx) -> lxb_status_t {
          string *content = reinterpret_cast<string *>(ctx);
          content->append(reinterpret_cast<const char *>(data), len);
          return LXB_STATUS_OK;
        },
        &tagString);

    string result;
    re2::StringPiece input(tagString);
    if (RE2::PartialMatch(
            input, RE2(attribute + "\\s*=\\s*['\"]([^'\"]+)['\"]"), &result))
      return result;

    return "";
  }

  string content() {
    string result = this->rawContent();
    RE2::GlobalReplace(&result, RE2("<[^>]*>.*</[^>]*>"), "");
    return result;

    return result;
  }

private:
  lxb_status_t status;
  lxb_dom_node_t *body;
  lxb_html_document_t *document;
  lxb_css_parser_t *parser;
  lxb_selectors_t *selectors;
  lxb_css_selector_list_t *list;
};
