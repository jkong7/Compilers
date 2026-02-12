#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "L3.h"
#include "tree.h"

namespace L3 {

  class Emitter {
  public:
    explicit Emitter(std::ostream& out);

    void line(const std::string& s);
    int64_t num_instructions() const;
    std::string fresh_tmp(); 

  private:
    std::ostream& out_;
    int64_t num_instructions_ = 0;
    int64_t tmp_next_ = 0; 
  };

  struct Match {
    const Tree* node = nullptr;

    const Tree* dst = nullptr;
    const Tree* lhs = nullptr;
    const Tree* rhs = nullptr;

    std::optional<OP>  op;
    std::optional<CMP> cmp;
  };

  struct GlobalLabel {
    void enter_function(const std::string& fn);
    std::string make_label(const std::string& l3_label); 
    std::string make_fresh_label(); 

    std::string prefix = ":global_";
    int64_t next = 0;
    std::string cur_fn;
    std::unordered_map<std::string, int64_t> labelMap;
};


  class Tile {
  public:
    virtual ~Tile() = default;

    virtual bool match(const Tree& t, Match& m) = 0;
    virtual int cost() = 0;
    virtual void emit(const Match& m, Emitter& e, GlobalLabel& labeler) = 0;
  };

  class AssignTile : public Tile {
    public: 
      bool match(const Tree& t, Match& m) override; 
      int cost() override; 
      void emit(const Match& m, Emitter& e, GlobalLabel& labeler) override;
  };

  class AssignBinOpTile : public Tile {
    public: 
      bool match(const Tree& t, Match& m) override; 
      int cost() override; 
      void emit(const Match& m, Emitter& e, GlobalLabel& labeler) override;
  };

  class AssignCmpTile : public Tile {
    public: 
      bool match(const Tree& t, Match& m) override; 
      int cost() override; 
      void emit(const Match& m, Emitter& e, GlobalLabel& labeler) override;
  };

  class LoadTile : public Tile {
    public: 
      bool match(const Tree& t, Match& m) override; 
      int cost() override; 
      void emit(const Match& m, Emitter& e, GlobalLabel& labeler) override;
  };

  class StoreTile : public Tile {
    public: 
      bool match(const Tree& t, Match& m) override; 
      int cost() override; 
      void emit(const Match& m, Emitter& e, GlobalLabel& labeler) override;
  };

  class ReturnTile : public Tile {
    public: 
      bool match(const Tree& t, Match& m) override; 
      int cost() override; 
      void emit(const Match& m, Emitter& e, GlobalLabel& labeler) override;
  };

  class BreakTile : public Tile {
    public: 
      bool match(const Tree& t, Match& m) override; 
      int cost() override; 
      void emit(const Match& m, Emitter& e, GlobalLabel& labeler) override;
  };












  class TilingEngine {
  public:
    explicit TilingEngine(std::ostream& out, GlobalLabel& labeler);
    void add_tile(std::unique_ptr<Tile> t);
    void tile(Program& p);

  private:
    void tile_function(Function& f);
    void tile_tree(const Tree& t);
    Tile* select_best_tile(const Tree& t, Match& out_match) const;

    Emitter emitter_;
    GlobalLabel labeler_; 
    std::vector<std::unique_ptr<Tile>> tiles_;
  };

  void tile_program(Program& p, std::ostream& out);

} 
