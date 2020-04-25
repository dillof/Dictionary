/*
  Implementation of the Dictionary data type
  for String key-value pairs, based on 
  CRC32/64 has keys and binary tree search

  ---
  
  Copyright (C) Anatoli Arkhipenko, 2020
  All rights reserved.
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.

  ---

  v1.0.0:
    2020-04-09 - Initial release

  v1.0.1:
    2020-04-10 - feature: operator (), examples, benchmarks
    
  v1.0.2:
    2020-04-10 - feature: operators == and !=
                 bug: memory leak after destroy method call. 
                 
  v1.1.0:
    2020-04-12 - feature: delete a node method.
                 feature: Dictionary Array optimization
                 
  v1.1.1:
    2020-04-13 - feature: check if key exists via d("key")
    
  v1.2.0:
    2020-04-25 - bug: incorrect node handling during deletion
                 performance improvements

*/


#ifndef _DICTIONARY_H_
#define _DICTIONARY_H_

#include "NodeArray.h"


//#define _DICT_CRC64_

class Dictionary {
  public:
    Dictionary(size_t init_size = 10);
    ~Dictionary();

    void insert(String keystr, String valstr);
    String search(String keystr);
    void destroy();
    void remove(String keystr);

    String key(unsigned int i) {
      node* p = (*Q)[i];
      if (p) return p->keystr;
      else return String();
    }
    String value(unsigned int i) {
      node* p = (*Q)[i];
      if (p) return p->valstr;
      else return String();
    }
    String operator [] (String keystr) {
      return search(keystr);
    }
    String operator [] (unsigned int i) {
      return value(i);
    }
    void operator () (String keystr, String valstr) {
      insert (keystr, valstr);
    }
    
    bool operator () (String keystr);
    
    String operator () (unsigned int i) {
      return key(i);
    }
    bool operator == (Dictionary& b);
    inline bool operator != (Dictionary& b) {
      return ( !(*this == b));
    }
    inline const size_t count() {
      return Q->count();
    }

    size_t size();

#ifdef _LIBDEBUG_
    void printNode(node* root);
    void printDictionary(node* root);
    void printDictionary() { 
      Serial.printf("\nDictionary::printDictionary:\n");
      printDictionary(iRoot); 
      Serial.println();
    };
    void printArray() { Q->printArray(); };
#endif
  private:
    void     destroy_tree(node* leaf);
    void     insert(uintNN_t key, String* keystr, String* valstr, node* leaf);
    node*    search(uintNN_t key, node* leaf, String* keystr);
    uintNN_t crc(const void* data, size_t n_bytes);

    node*    deleteNode(node* root, uintNN_t key, String* keystr);
    node*    minValueNode(node* n);

    node*               iRoot;
    uintNN_t            table[0x100];
    NodeArray*          Q;
    size_t              initSize;
};


Dictionary::Dictionary(size_t init_size) {
  iRoot = NULL;

#ifdef _DICT_CRC64_
  const uintNN_t m_poly = 0xC96C5795D7870F42ull;
  for (int i = 0; i < 256; ++i) {
    uintNN_t crc = i;
    for (int j = 0; j < 8; ++j) {
      if (crc & 1) { // is current coefficient set?
        crc >>= 1;   // yes, then assume it gets zero'd (by implied x^64 coefficient of dividend)
        crc ^= m_poly; // and add rest of the divisor
      }
      else { // no? then move to next coefficient
        crc >>= 1;
      }
    }
    table[i] = crc;
  }
#else
  for (size_t i = 0; i < 0x100; ++i) {
    uintNN_t r = i;
    for (int j = 0; j < 8; ++j) {
      r = (r & 1 ? 0 : (uintNN_t)0xEDB88320L) ^ r >> 1;
    }
    table[i] = ( r ^ (uintNN_t)0xFF000000L );
  }
#endif

  Q = new NodeArray(init_size);
  initSize = init_size;
}


Dictionary::~Dictionary() {
  destroy();
  delete Q;
}

void Dictionary::destroy_tree(node *leaf) {
  if (leaf != NULL) {
    destroy_tree(leaf->left);
    destroy_tree(leaf->right);
    delete leaf;
  }
}

void Dictionary::insert(uintNN_t key, String *keystr, String *valstr, node *leaf) {
  if (key < leaf->key) {
    if (leaf->left != NULL)
      insert(key, keystr, valstr, leaf->left);
    else {
      leaf->left = new node;
      leaf->left->key = key;
      leaf->left->keystr = *keystr;
      leaf->left->valstr = *valstr;
      leaf->left->left = NULL;  //Sets the left child of the child node to null
      leaf->left->right = NULL; //Sets the right child of the child node to null
      Q->append(leaf->left);
    }
  }
  else if (key > leaf->key) {
    if (leaf->right != NULL)
      insert(key, keystr, valstr, leaf->right);
    else {
      leaf->right = new node;
      leaf->right->key = key;
      leaf->right->keystr = *keystr;
      leaf->right->valstr = *valstr;
      leaf->right->left = NULL; //Sets the left child of the child node to null
      leaf->right->right = NULL; //Sets the right child of the child node to null
      Q->append(leaf->right);
    }
  }
  else if (key == leaf->key) {
    if ( leaf->keystr == *keystr ) {
      leaf->valstr = *valstr;
    }
    else {
      if (leaf->right != NULL)
        insert(key, keystr, valstr, leaf->right);
      else {
        leaf->right = new node;
        leaf->right->key = key;
        leaf->right->keystr = *keystr;
        leaf->right->valstr = *valstr;
        leaf->right->left = NULL; //Sets the left child of the child node to null
        leaf->right->right = NULL; //Sets the right child of the child node to null
        Q->append(leaf->right);
      }
    }
  }
}

bool Dictionary::operator () (String keystr) {
  uintNN_t key = crc(keystr.c_str(), keystr.length());

  node *p = search(key, iRoot, &keystr);
  if (p) return true;
  return false;
}


node *Dictionary::search(uintNN_t key, node *leaf, String* keystr) {
  if (leaf != NULL) {
    if (key == leaf->key && leaf->keystr == *keystr)
      return leaf;
    if (key < leaf->key)
      return search(key, leaf->left, keystr);
    else
      return search(key, leaf->right, keystr);
  }
  else return NULL;
}


void Dictionary::insert(String keystr, String valstr) {
  uintNN_t key = crc( keystr.c_str(), keystr.length() );

  if (iRoot != NULL)
    insert(key, &keystr, &valstr, iRoot);
  else {
    iRoot = new node;
    iRoot->key = key;
    iRoot->keystr = keystr;
    iRoot->valstr = valstr;
    iRoot->left = NULL;
    iRoot->right = NULL;
    Q->append(iRoot);
  }
}


String Dictionary::search(String keystr) {
  uintNN_t key = crc(keystr.c_str(), keystr.length());

  node *p = search(key, iRoot, &keystr);
  if (p) return p->valstr;
  return String("");
}


void Dictionary::destroy() {
  destroy_tree(iRoot);
  delete Q;
  Q = new NodeArray(initSize);
}


size_t Dictionary::size() {
  size_t ct = count();
  size_t sz = 0;
  for (size_t i = 0; i < ct; i++) {
    sz += key(i).length();
    sz += value(i).length();
    sz += 2;  // to account for the 2 trailing zeros
  }
  return sz;
}

bool Dictionary::operator == (Dictionary& b) {
  if (b.size() != size() ) return false;
  if (b.count() != count() ) return false;
  size_t ct = count();
  for (size_t i = 0; i < ct; i++) {
    if ( value(i) != b[key(i)] ) return false;
  }
  return true;
}


void Dictionary::remove(String keystr) {
#ifdef _LIBDEBUG_
  Serial.printf( "Dictionary::remove: %s\n", keystr.c_str() );
#endif
  
  uintNN_t key = crc(keystr.c_str(), keystr.length());
  node *p = search(key, iRoot, &keystr);

  if (p) { 
#ifdef _LIBDEBUG_
  Serial.printf( "Found key to delete int: %u\n", p->key );
  Serial.printf( "Found key to delete ptr: %u\n", (uint32_t)p );
  Serial.printf( "Found key to delete str: %s\n", keystr.c_str() );
#endif
    iRoot = deleteNode(iRoot, p->key, &keystr);
  }
}

node* Dictionary::deleteNode(node* root, uintNN_t key, String* keystr) {
  if (root == NULL) return root;

  if (key < root->key)
    root->left = deleteNode(root->left, key, keystr);

  // If the key to be deleted is greater than the root's key,
  // then it lies in right subtree
  else if (key > root->key)
    root->right = deleteNode(root->right, key, keystr);

  // if key is same as root's key, then This is the node
  // to be deleted
  else {
    if (root->keystr == *keystr) {
      // node with only one child or no child
      if (root->left == NULL) {
#ifdef _LIBDEBUG_
  Serial.println("Replacing RIGHT node");
  printNode(root);
  printNode(root->right);
#endif        
        node* temp = root->right;
        Q->remove(root);
        delete root;
        root = NULL;
        return temp;
      } 
      else if (root->right == NULL) {
#ifdef _LIBDEBUG_
  Serial.println("Replacing LEFT node");
  printNode(root);
  printNode(root->left);
#endif 
        node* temp = root->left;
        Q->remove(root);
        delete root;
        root = NULL;
        return temp;
      }

      // node with two children: Get the inorder successor (smallest
      // in the right subtree)
      node* temp = minValueNode(root->right);
#ifdef _LIBDEBUG_
  Serial.println("Replacing minValueNode node");
  printNode(root);
  printNode(temp);
#endif 

      // Copy the inorder successor's content to this node
      root->key = temp->key;
      root->keystr = temp->keystr;
      root->valstr = temp->valstr;

      // Delete the inorder successor
      root->right = deleteNode(root->right, temp->key, &temp->keystr);
    }
    else {
      root->right = deleteNode(root->right, key, keystr);
    }
  }
  return root;
}


node* Dictionary::minValueNode(node* n) {
  node* current = n;

  /* loop down to find the leftmost leaf */
  while (current && current->left != NULL)
    current = current->left;

  return current;
}


#ifdef _LIBDEBUG_
void Dictionary::printDictionary(node* root) { 
    if (root != NULL) 
    { 
        printDictionary(root->left); 
        printNode(root);
        printDictionary(root->right); 
    } 
} 

void Dictionary::printNode(node* root)  { 
    if (root != NULL) { 
        Serial.printf("%u: (%u:%s) [l:%u, r:%u]\n", (uint32_t)root, root->key, (root->keystr).c_str(), (uint32_t)root->left, (uint32_t)root->right);
    } 
    else {
        Serial.println("NULL:");
    }
}
#endif

#ifdef _DICT_CRC64_
uintNN_t Dictionary::crc(const void *p, size_t len) {
  size_t i, t;
  uintNN_t crc = 0;
  uint8_t *_p = (uint8_t*)p;

  for (i = 0; i < len; i++) {
    t = ((crc >> 56) ^ (*_p++)) & 0xFF;
    crc = table[t] ^ (crc << 8);
  }
  return crc;
}
#else
uintNN_t Dictionary::crc(const void *data, size_t n_bytes) {
  uintNN_t crc = 0;
  for (size_t i = 0; i < n_bytes; ++i) {
    crc = table[(uint8_t)crc ^ ((uint8_t*)data)[i]] ^ crc >> 8;
  }
  return crc;
}
#endif

#endif // #define _DICTIONARY_H_