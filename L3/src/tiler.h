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

  private:
    std::ostream& out_;
    int64_t num_instructions_ = 0;
  };

  struct Match {
    const Tree* node = nullptr;

    const Tree* dst = nullptr;
    const Tree* lhs = nullptr;
    const Tree* rhs = nullptr;

    std::optional<OP>  op;
    std::optional<CMP> cmp;
  };



  class Tile {
  public:
    virtual ~Tile() = default;

    virtual bool match(const Tree& t, Match& m) = 0;
    virtual int cost() = 0;
    virtual void emit(const Match& m, Emitter& e) = 0;
  };

  class AssignTile : public Tile {
    public: 
      bool match(const Tree& t, Match& m) override; 
      int cost() override; 
      void emit(const Match& m, Emitter& e) override;
  };

  class AssignBinOpTile : public Tile {
    public: 
      bool match(const Tree& t, Match& m) override; 
      int cost() override; 
      void emit(const Match& m, Emitter& e) override;
  };

  class AssignCmpTile : public Tile {
    public: 
      bool match(const Tree& t, Match& m) override; 
      int cost() override; 
      void emit(const Match& m, Emitter& e) override;
  };

  class LoadTile : public Tile {
    public: 
      bool match(const Tree& t, Match& m) override; 
      int cost() override; 
      void emit(const Match& m, Emitter& e) override;
  };

  class StoreTile : public Tile {
    public: 
      bool match(const Tree& t, Match& m) override; 
      int cost() override; 
      void emit(const Match& m, Emitter& e) override;
  };

  class ReturnTile : public Tile {
    public: 
      bool match(const Tree& t, Match& m) override; 
      int cost() override; 
      void emit(const Match& m, Emitter& e) override;
  };

  class BreakTile : public Tile {
    public: 
      bool match(const Tree& t, Match& m) override; 
      int cost() override; 
      void emit(const Match& m, Emitter& e) override;
  };












  class TilingEngine {
  public:
    explicit TilingEngine(std::ostream& out);
    void add_tile(std::unique_ptr<Tile> t);
    void tile(Program& p);

  private:
    void tile_function(Function& f);
    void tile_tree(const Tree& t);
    Tile* select_best_tile(const Tree& t, Match& out_match) const;

    Emitter emitter_;
    std::vector<std::unique_ptr<Tile>> tiles_;
  };

  void tile_program(Program& p, std::ostream& out);

} 
