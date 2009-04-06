/*
** Storage.h
**
*/

#ifndef   	STORAGE_H_
#define   	STORAGE_H_

#include <malloc.h>
#include <cassert>
#include <cstring>
#include <iostream>
#include <sstream>
#include <map>
#include <iterator>
#include <exception>

using namespace std;

class MsaException : public std::exception {
	public:
		string msg;
		MsaException(const char* msg)
		{
			this->msg = msg;
		}

		virtual ~MsaException() throw() {}
		virtual const char* what()
		{
			return msg.c_str();
		}
};

typedef enum
{
    A = 0,
    C,
    G,
    T
} GeneticSymbols;

static char gs2cLookup[] = {'A', 'C', 'G', 'T'};

__inline__ char toChar(GeneticSymbols gs)
{
    return gs2cLookup[(int)gs];
}

__inline__ GeneticSymbols fromChar(char c)
{
    switch (c)
    {
    case 'A':
    case 'a':
    	return A;
    case 'C':
    case 'c':
    	return C;
    case 'G':
    case 'g':
    	return G;
    case 'T':
    case 't':
    	return T;
    default:
    	throw new MsaException("Invalid GS character");
    }
}


template <class T>
void parse(T** data, size_t* size, istream&);

template <class T>
class MutableSequence;

template <class T>
class ImmutableSequence
{
    friend class MutableSequence<T>;
    T* seq;
    size_t seqSize;

    ImmutableSequence(T* seq, size_t size)
	: seq(seq), seqSize(size) {}
    
public:
    ImmutableSequence(istream s)
    {
    	parse(&seq, &seqSize, s);
    }

    ImmutableSequence(string seqstr)
    {
    	seqSize = seqstr.length();
    	stringstream ss(stringstream::in | stringstream::out);
    	ss << seqSize << seqstr;
    	parse(&seq, &seqSize, ss);
    }

    ~ImmutableSequence()
    {
    	free(this->seq);
    }

    T at(size_t idx)
    {
    	assert(idx <= seqSize);
    	return seq[idx];
    }

    size_t length()
    {
    	return seqSize;
    }

    string toString()
    {
    	char buf[seqSize+1];
    	for (size_t i=0; i<seqSize; i++)
    	{
    		buf[i] = toChar(seq[i]);
    	}
    	buf[seqSize] = 0;
    	return string(buf);
    }

    bool operator==(ImmutableSequence& o)
    {
    	if (o.seqSize != seqSize)
    		return false;
    	return (memcmp(o.seq, seq, seqSize) == 0);
    }
};

struct Change {
    bool ins;
    bool del;
    size_t t;

    Change(bool ins, bool del, size_t t)
    : ins(ins), del(del), t(t) {}

    Change() {}
};

template <class T>
class MutableSequence
{
    typedef map<size_t, struct Change> ChangeMap;

    ImmutableSequence<T>& seq;
    ChangeMap changes;
     
public:

    MutableSequence(ImmutableSequence<T>& seq) 
	: seq(seq) {}
    void set(size_t idx, T t) {
	changes[idx] = Change(false, false, (size_t)t);
    }

    //Insert a character before index
    void insert(size_t idx, T t) {
	changes[idx] = Change(true, false, (size_t)t);
    }

    //Delete index
    void del(size_t idx) {
	changes[idx] = Change(false, true, 0);
    }

    //Create an immutable sequence from this
    ImmutableSequence<T>* commit() {
	size_t size = length();
	T *arr = (T*)malloc(sizeof(T) * size);
	size_t nPos = 0, oPos = 0;
	
	ChangeMap::iterator iter;
	for (iter = changes.begin(); iter != changes.end(); iter++) {
	    size_t cLen = iter->first - oPos;
	    if ((oPos + cLen) >= seq.seqSize)
		cLen = seq.seqSize - oPos;
	    
	    memcpy(&arr[nPos], &seq.seq[oPos], cLen*sizeof(T));

	    if (iter->second.del) {
		nPos += cLen;
		oPos += cLen + 1;
	    } else if (iter->second.ins) {
		nPos += cLen;
		arr[nPos] = (T)iter->second.t;
		nPos++;
		oPos += cLen;
	    } else {
		nPos += cLen;
		oPos += cLen;
		arr[nPos] = (T)iter->second.t;
		nPos++;
		oPos++;
	    }
	}

	size_t cLen = seq.seqSize - changes.rbegin()->first;	
	memcpy(&arr[nPos], &seq.seq[oPos], cLen*sizeof(T));

	return new ImmutableSequence<T>(arr, size);
    }

    size_t length() {
	int add = 0;

	for (ChangeMap::iterator i = changes.begin(); i != changes.end(); i++) {
	    if (i->second.del)
		--add;
	    if (i->second.ins)
		++add;
	}

	return seq.length() + add;
    }

};

template <class T, int S>
class ScoringMatrix {
public:
    double matrix[S*S];
    double alpha;
    double beta;

    ScoringMatrix() {}

    ScoringMatrix(double match, double nonmatch, double alpha, double beta)
    {
    	for (size_t i = 0; i<(S*S); i++)
    	{
    		matrix[i] = nonmatch;
    	}

    	for (size_t i=0; i<S; i++)
    	{
    		matrix[i*S + i] = match;
    	}

	this->alpha = alpha;
	this->beta = beta;
    }

    ScoringMatrix(double alpha, double beta)
    {
    	this->alpha = alpha;
    	this->beta = beta;
    }

    double score(T a, T b)
    {
    	return matrix[a*S + b];
    }

    double affineScore(int gapSize)
    {
    	if(gapSize < 1)
    	{
    		return 0;
    	}
    	return -1 * (alpha + beta*(gapSize-1));
    }
};

typedef ScoringMatrix<GeneticSymbols, 4> GenScores;
typedef ImmutableSequence<GeneticSymbols> GISeq;
typedef MutableSequence<GeneticSymbols> MGISeq;

#endif 	    /* !STORAGE_H_ */