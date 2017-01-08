// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Finite-State Transducer (FST) archive classes.

#ifndef FST_EXTENSIONS_FAR_FAR_H_
#define FST_EXTENSIONS_FAR_FAR_H_

#include <iostream>
#include <sstream>

#include <fst/extensions/far/stlist.h>
#include <fst/extensions/far/sttable.h>
#include <fst/fst.h>
#include <fst/vector-fst.h>
#include <fstream>

namespace fst {

enum FarEntryType { FET_LINE, FET_FILE };
enum FarTokenType { FTT_SYMBOL, FTT_BYTE, FTT_UTF8 };

inline bool IsFst(const string &filename) {
  std::ifstream strm(filename.c_str(),
                          std::ios_base::in | std::ios_base::binary);
  if (!strm) return false;
  return IsFstHeader(strm, filename);
}

// FST archive header class
class FarHeader {
 public:
  const string &FarType() const { return fartype_; }
  const string &ArcType() const { return arctype_; }

  bool Read(const string &filename) {
    FstHeader fsthdr;
    if (filename.empty()) {
      // Header reading unsupported on stdin. Assumes STList and StdArc.
      fartype_ = "stlist";
      arctype_ = "standard";
      return true;
    } else if (IsSTTable(filename)) {  // Check if STTable
      ReadSTTableHeader(filename, &fsthdr);
      fartype_ = "sttable";
      arctype_ = fsthdr.ArcType().empty() ? "unknown" : fsthdr.ArcType();
      return true;
    } else if (IsSTList(filename)) {  // Check if STList
      ReadSTListHeader(filename, &fsthdr);
      fartype_ = "stlist";
      arctype_ = fsthdr.ArcType().empty() ? "unknown" : fsthdr.ArcType();
      return true;
    } else if (IsFst(filename)) {  // Check if Fst
      std::ifstream istrm(filename.c_str(),
                               std::ios_base::in | std::ios_base::binary);
      fsthdr.Read(istrm, filename);
      fartype_ = "fst";
      arctype_ = fsthdr.ArcType().empty() ? "unknown" : fsthdr.ArcType();
      return true;
    }
    return false;
  }

 private:
  string fartype_;
  string arctype_;
};

enum FarType {
  FAR_DEFAULT = 0,
  FAR_STTABLE = 1,
  FAR_STLIST = 2,
  FAR_FST = 3,
};

// This class creates an archive of FSTs.
template <class A>
class FarWriter {
 public:
  typedef A Arc;

  // Creates a new (empty) FST archive; returns null on error.
  static FarWriter *Create(const string &filename, FarType type = FAR_DEFAULT);

  // Adds an FST to the end of an archive. Keys must be non-empty and
  // in lexicographic order. FSTs must have a suitable write method.
  virtual void Add(const string &key, const Fst<A> &fst) = 0;

  virtual FarType Type() const = 0;

  virtual bool Error() const = 0;

  virtual ~FarWriter() {}

 protected:
  FarWriter() {}
};

// This class iterates through an existing archive of FSTs.
template <class A>
class FarReader {
 public:
  typedef A Arc;

  // Opens an existing FST archive in a single file; returns null on error.
  // Sets current position to the beginning of the achive.
  static FarReader *Open(const string &filename);

  // Opens an existing FST archive in multiple files; returns null on error.
  // Sets current position to the beginning of the achive.
  static FarReader *Open(const std::vector<string> &filenames);

  // Resets current position to beginning of archive.
  virtual void Reset() = 0;

  // Sets current position to first entry >= key.  Returns true if a match.
  virtual bool Find(const string &key) = 0;

  // Current position at end of archive?
  virtual bool Done() const = 0;

  // Move current position to next FST.
  virtual void Next() = 0;

  // Returns key at the current position. This reference is invalidated if
  // the current position in the archive is changed.
  virtual const string &GetKey() const = 0;

  // Returns pointer to FST at the current position. This is invalidated if
  // the current position in the archive is changed.
  virtual const Fst<A> *GetFst() const = 0;

  virtual FarType Type() const = 0;

  virtual bool Error() const = 0;

  virtual ~FarReader() {}

 protected:
  FarReader() {}
};

template <class A>
class FstWriter {
 public:
  void operator()(std::ostream &strm, const Fst<A> &fst) const {
    fst.Write(strm, FstWriteOptions());
  }
};

template <class A>
class STTableFarWriter : public FarWriter<A> {
 public:
  typedef A Arc;

  static STTableFarWriter *Create(const string &filename) {
    STTableWriter<Fst<A>, FstWriter<A>> *writer =
        STTableWriter<Fst<A>, FstWriter<A>>::Create(filename);
    return new STTableFarWriter(writer);
  }

  void Add(const string &key, const Fst<A> &fst) override {
    writer_->Add(key, fst);
  }

  FarType Type() const override { return FAR_STTABLE; }

  bool Error() const override { return writer_->Error(); }

 private:
  explicit STTableFarWriter(STTableWriter<Fst<A>, FstWriter<A>> *writer)
      : writer_(writer) {}

  std::unique_ptr<STTableWriter<Fst<A>, FstWriter<A>>> writer_;
};

template <class A>
class STListFarWriter : public FarWriter<A> {
 public:
  typedef A Arc;

  static STListFarWriter *Create(const string &filename) {
    STListWriter<Fst<A>, FstWriter<A>> *writer =
        STListWriter<Fst<A>, FstWriter<A>>::Create(filename);
    return new STListFarWriter(writer);
  }

  void Add(const string &key, const Fst<A> &fst) override {
    writer_->Add(key, fst);
  }

  FarType Type() const override { return FAR_STLIST; }

  bool Error() const override { return writer_->Error(); }

 private:
  explicit STListFarWriter(STListWriter<Fst<A>, FstWriter<A>> *writer)
      : writer_(writer) {}

  std::unique_ptr<STListWriter<Fst<A>, FstWriter<A>>> writer_;
};

template <class A>
class FstFarWriter : public FarWriter<A> {
 public:
  typedef A Arc;

  explicit FstFarWriter(const string &filename)
      : filename_(filename), error_(false), written_(false) {}

  static FstFarWriter *Create(const string &filename) {
    return new FstFarWriter(filename);
  }

  void Add(const string &key, const Fst<A> &fst) override {
    if (written_) {
      LOG(WARNING) << "FstFarWriter::Add: only one Fst supported,"
                   << " subsequent entries discarded.";
    } else {
      error_ = !fst.Write(filename_);
      written_ = true;
    }
  }

  FarType Type() const override { return FAR_FST; }

  bool Error() const override { return error_; }

  ~FstFarWriter() override {}

 private:
  string filename_;
  bool error_;
  bool written_;
};

template <class A>
FarWriter<A> *FarWriter<A>::Create(const string &filename, FarType type) {
  switch (type) {
    case FAR_DEFAULT:
      if (filename.empty()) return STListFarWriter<A>::Create(filename);
    case FAR_STTABLE:
      return STTableFarWriter<A>::Create(filename);
    case FAR_STLIST:
      return STListFarWriter<A>::Create(filename);
    case FAR_FST:
      return FstFarWriter<A>::Create(filename);
    default:
      LOG(ERROR) << "FarWriter::Create: Unknown FAR type";
      return nullptr;
  }
}

template <class A>
class FstReader {
 public:
  Fst<A> *operator()(std::istream &strm) const {
    return Fst<A>::Read(strm, FstReadOptions());
  }
};

template <class A>
class STTableFarReader : public FarReader<A> {
 public:
  typedef A Arc;

  static STTableFarReader *Open(const string &filename) {
    STTableReader<Fst<A>, FstReader<A>> *reader =
        STTableReader<Fst<A>, FstReader<A>>::Open(filename);
    // TODO: error check
    return new STTableFarReader(reader);
  }

  static STTableFarReader *Open(const std::vector<string> &filenames) {
    STTableReader<Fst<A>, FstReader<A>> *reader =
        STTableReader<Fst<A>, FstReader<A>>::Open(filenames);
    // TODO: error check
    return new STTableFarReader(reader);
  }

  void Reset() override { reader_->Reset(); }

  bool Find(const string &key) override { return reader_->Find(key); }

  bool Done() const override { return reader_->Done(); }

  void Next() override { return reader_->Next(); }

  const string &GetKey() const override { return reader_->GetKey(); }

  const Fst<A> *GetFst() const override { return reader_->GetEntry(); }

  FarType Type() const override { return FAR_STTABLE; }

  bool Error() const override { return reader_->Error(); }

 private:
  explicit STTableFarReader(STTableReader<Fst<A>, FstReader<A>> *reader)
      : reader_(reader) {}

  std::unique_ptr<STTableReader<Fst<A>, FstReader<A>>> reader_;
};

template <class A>
class STListFarReader : public FarReader<A> {
 public:
  typedef A Arc;

  static STListFarReader *Open(const string &filename) {
    STListReader<Fst<A>, FstReader<A>> *reader =
        STListReader<Fst<A>, FstReader<A>>::Open(filename);
    // TODO: error check
    return new STListFarReader(reader);
  }

  static STListFarReader *Open(const std::vector<string> &filenames) {
    STListReader<Fst<A>, FstReader<A>> *reader =
        STListReader<Fst<A>, FstReader<A>>::Open(filenames);
    // TODO: error check
    return new STListFarReader(reader);
  }

  void Reset() override { reader_->Reset(); }

  bool Find(const string &key) override { return reader_->Find(key); }

  bool Done() const override { return reader_->Done(); }

  void Next() override { return reader_->Next(); }

  const string &GetKey() const override { return reader_->GetKey(); }

  const Fst<A> *GetFst() const override { return reader_->GetEntry(); }

  FarType Type() const override { return FAR_STLIST; }

  bool Error() const override { return reader_->Error(); }

 private:
  explicit STListFarReader(STListReader<Fst<A>, FstReader<A>> *reader)
      : reader_(reader) {}

  std::unique_ptr<STListReader<Fst<A>, FstReader<A>>> reader_;
};

template <class A>
class FstFarReader : public FarReader<A> {
 public:
  typedef A Arc;

  static FstFarReader *Open(const string &filename) {
    std::vector<string> filenames;
    filenames.push_back(filename);
    return new FstFarReader<A>(filenames);
  }

  static FstFarReader *Open(const std::vector<string> &filenames) {
    return new FstFarReader<A>(filenames);
  }

  explicit FstFarReader(const std::vector<string> &filenames)
      : keys_(filenames), has_stdin_(false), pos_(0), error_(false) {
    std::sort(keys_.begin(), keys_.end());
    streams_.resize(keys_.size(), 0);
    for (size_t i = 0; i < keys_.size(); ++i) {
      if (keys_[i].empty()) {
        if (!has_stdin_) {
          streams_[i] = &std::cin;
          // sources_[i] = "stdin";
          has_stdin_ = true;
        } else {
          FSTERROR() << "FstFarReader::FstFarReader: stdin should only "
                     << "appear once in the input file list.";
          error_ = true;
          return;
        }
      } else {
        streams_[i] = new std::ifstream(
            keys_[i].c_str(), std::ios_base::in | std::ios_base::binary);
      }
    }
    if (pos_ >= keys_.size()) return;
    ReadFst();
  }

  void Reset() override {
    if (has_stdin_) {
      FSTERROR() << "FstFarReader::Reset: Operation not supported on stdin";
      error_ = true;
      return;
    }
    pos_ = 0;
    ReadFst();
  }

  bool Find(const string &key) override {
    if (has_stdin_) {
      FSTERROR() << "FstFarReader::Find: Operation not supported on stdin";
      error_ = true;
      return false;
    }
    pos_ = 0;  // TODO
    ReadFst();
    return true;
  }

  bool Done() const override { return error_ || pos_ >= keys_.size(); }

  void Next() override {
    ++pos_;
    ReadFst();
  }

  const string &GetKey() const override { return keys_[pos_]; }

  const Fst<A> *GetFst() const override { return fst_.get(); }

  FarType Type() const override { return FAR_FST; }

  bool Error() const override { return error_; }

  ~FstFarReader() override {
    for (size_t i = 0; i < keys_.size(); ++i) {
      if (streams_[i] != &std::cin) {
        delete streams_[i];
      }
    }
  }

 private:
  void ReadFst() {
    fst_.reset();
    if (pos_ >= keys_.size()) return;
    streams_[pos_]->seekg(0);
    fst_.reset(Fst<A>::Read(*streams_[pos_], FstReadOptions()));
    if (!fst_) {
      FSTERROR() << "FstFarReader: Error reading Fst from: " << keys_[pos_];
      error_ = true;
    }
  }

  std::vector<string> keys_;
  std::vector<std::istream *> streams_;
  bool has_stdin_;
  size_t pos_;
  mutable std::unique_ptr<Fst<A>> fst_;
  mutable bool error_;
};

template <class A>
FarReader<A> *FarReader<A>::Open(const string &filename) {
  if (filename.empty())
    return STListFarReader<A>::Open(filename);
  else if (IsSTTable(filename))
    return STTableFarReader<A>::Open(filename);
  else if (IsSTList(filename))
    return STListFarReader<A>::Open(filename);
  else if (IsFst(filename))
    return FstFarReader<A>::Open(filename);
  return nullptr;
}

template <class A>
FarReader<A> *FarReader<A>::Open(const std::vector<string> &filenames) {
  if (!filenames.empty() && filenames[0].empty())
    return STListFarReader<A>::Open(filenames);
  else if (!filenames.empty() && IsSTTable(filenames[0]))
    return STTableFarReader<A>::Open(filenames);
  else if (!filenames.empty() && IsSTList(filenames[0]))
    return STListFarReader<A>::Open(filenames);
  else if (!filenames.empty() && IsFst(filenames[0]))
    return FstFarReader<A>::Open(filenames);
  return nullptr;
}

}  // namespace fst

#endif  // FST_EXTENSIONS_FAR_FAR_H_
