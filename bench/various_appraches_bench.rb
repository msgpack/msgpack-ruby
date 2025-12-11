#!/usr/bin/env ruby
# frozen_string_literal: true

require "bundler/inline"

gemfile(true) do
  source "https://rubygems.org"
  if ENV["MSGPACK_PATH"]
    gem "msgpack", path: ENV["MSGPACK_PATH"]
  else
    gem "msgpack", git: "https://github.com/Shopify/msgpack-ruby.git", branch: "gm/ref-tracking"
  end
  gem "benchmark-ips"
end

require "msgpack"
require "benchmark"

RubyVM::YJIT.enable

begin
  factory = MessagePack::Factory.new
  test_struct = Struct.new(:a, keyword_init: true)
  factory.register_type(0x01, test_struct, optimized_struct: true, ref_tracking: true)
  obj = test_struct.new(a: 1)
  arr = [obj, obj, obj]
  dump = factory.dump(arr)
  loaded = factory.load(dump)

  unless loaded[0].object_id == loaded[1].object_id
    puts "ERROR: msgpack-ruby does not support ref_tracking; the benchmark cannot run."
    exit(1)
  end
rescue StandardError
  puts "ERROR: msgpack-ruby does not support optimized_struct; the benchmark cannot run."
  exit(2)
end

#   +-----------------+
#   |     Product     |
#   +-----------------+
#      |             |            .variants
#      | .options    |----------------+------------------------------------|
#      v                              v                                    v
#   +----------------+      +--------------------+              +--------------------+
#   | ProductOption  |----->| ProductVariant[1]  |              | ProductVariant[2]  |
#   | (e.g. "Color") |      +--------------------+              +--------------------+
#   +----------------+         |           ^                      |                   ^
#      |                       | .options  | .selling_plans       | .options          | .selling_plans
#      | .values               |           |                      |                   |
#      v                       v           |                      v                   |
#   +----------------------+<--+           |              +----------------------+    |
#   | ProductOptionValue A | (SHARED)      |     (SHARED) | ProductOptionValue B |    |
#   | (e.g. "Red")         |               |              | (e.g. "Blue")        |    |
#   +----------------------+               |              +----------------------+    |
#                                          |                                          |
#                                          |                                          |
#   +----------------------+               |                                          |
#   | SellingPlanGroup X   |---------------+------------------------------------------|
#   +----------------------+
#      |
#      | .selling_plans
#      v
#   +----------------------+
#   |    SellingPlan[1]    | (SHARED, referenced by ProductVariant)
#   +----------------------+
# Metafield - matches real DataApi::Messages::Metafield (12 fields)
Metafield = Struct.new(
  :id,
  :namespace,
  :key,
  :value,
  :type,
  :value_type,
  :definition_id,
  :owner_type,
  :owner_id,
  :created_at,
  :updated_at,
  :original_type,
  keyword_init: true,
)

SellingPlanPriceAdjustment = Struct.new(
  :order_count,
  :position,
  :value_type,
  :value,
  keyword_init: true,
)

SellingPlanOption = Struct.new(
  :name,
  :position,
  :value,
  keyword_init: true,
)

SellingPlanCheckoutCharge = Struct.new(
  :value_type,
  :value,
  keyword_init: true,
)

# SellingPlan is SHARED - the same selling plan instance is referenced by
# multiple products and variants that offer the same subscription option
SellingPlan = Struct.new(
  :id,
  :name,
  :description,
  :recurring_deliveries,
  :options,
  :price_adjustments,
  :checkout_charge,
  keyword_init: true,
)

SellingPlanGroup = Struct.new(
  :id,
  :name,
  :options,
  :selling_plans,
  keyword_init: true,
)

# ProductOptionValue is SHARED - the same option value instance appears in
# both product.options[].values AND variant.options
ProductOptionValue = Struct.new(
  :id,
  :name,
  :position,
  :swatch_color,
  keyword_init: true,
)

ProductOption = Struct.new(
  :id,
  :name,
  :position,
  :values,
  keyword_init: true,
)

# ProductVariant - matches real ProductLoader::Messages::ProductVariant (37 fields)
# Many fields are nil in typical use, which affects serialization size
ProductVariant = Struct.new(
  :id,
  :product_id,
  :title,
  :uncontextualized_title,
  :price,
  :compare_at_price,
  :barcode,
  :options,           # References SHARED ProductOptionValue objects
  :option1,
  :option2,
  :option1_id,
  :option2_id,
  :parent_option_value_ids,
  :taxable,
  :unit_price_measurement,
  :position,
  :created_at,
  :updated_at,
  :fulfillment_service,
  :requires_components,
  :inventory_management,
  :inventory_policy,
  :weight_unit,
  :weight_value,
  :sku,
  :requires_shipping,
  :selling_plans,     # References SHARED SellingPlan objects
  :metafields,
  :variant_unit_price_measurement,
  keyword_init: true,
)

# Product - matches real ProductLoader::Messages::Product (28 fields)
# Many fields are nil in typical use, which affects serialization size
Product = Struct.new(
  :id,
  :title,
  :handle,
  :description,
  :type,
  :vendor,
  :published_at,
  :created_at,
  :updated_at,
  :template_suffix,
  :gift_card,
  :is_published,
  :requires_selling_plan,
  :published_scope,
  :variants,
  :options,              # Contains SHARED ProductOptionValue objects
  :selling_plan_groups,  # Contains SHARED SellingPlan objects
  :metafields,
  keyword_init: true,
)

ALL_STRUCTS = [
  Metafield,
  SellingPlanPriceAdjustment,
  SellingPlanOption,
  SellingPlanCheckoutCharge,
  SellingPlan,
  SellingPlanGroup,
  ProductOptionValue,
  ProductOption,
  ProductVariant,
  Product,
].freeze

# Struct types that are shared and benefit from ref_tracking
# - ProductOptionValue: shared between product.options[].values and variant.options
# - SellingPlan: shared between product.selling_plan_groups[].selling_plans and variant.selling_plans
# - SellingPlanGroup: can be shared across multiple products
# - ProductVariant: shared in combined listings
SHARED_STRUCTS = [
  SellingPlan,
  SellingPlanGroup,
  ProductOptionValue,
  ProductVariant,
].to_set.freeze

module CodeGen
  def self.build_tracked_packer(struct)
    packer_body = struct.members.map { |m| "packer.write(obj.#{m})" }.join("; ")

    eval(<<~RUBY, binding, __FILE__, __LINE__ + 1)
      ->(obj, packer) {
        tracker = packer.ref_tracker
        ref_id = tracker[obj.__id__]
        if ref_id
          packer.write(ref_id)
        else
          tracker[obj.__id__] = tracker.size + 1
          packer.write(nil)
          #{packer_body}
        end
      }
    RUBY
  end

  def self.build_untracked_packer(struct)
    packer_body = struct.members.map { |m| "packer.write(obj.#{m})" }.join("; ")
    eval(<<~RUBY, binding, __FILE__, __LINE__ + 1)
      ->(obj, packer) {
        #{packer_body}
      }
    RUBY
  end

  def self.build_tracked_unpacker(struct)
    args = struct.members.map { |m| "#{m}: unpacker.read" }.join(", ")

    eval(<<~RUBY, binding, __FILE__, __LINE__ + 1)
      ->(unpacker) {
        ref_id = unpacker.read
        if ref_id
          unpacker.ref_tracker[ref_id - 1]
        else
          tracker = unpacker.ref_tracker
          idx = tracker.size
          tracker << (obj = #{struct}.allocate)
          obj.send(:initialize, #{args})
          obj
        end
      }
    RUBY
  end

  def self.build_untracked_unpacker(struct)
    args = struct.members.map { |m| "#{m}: unpacker.read" }.join(", ")
    eval(<<~RUBY, binding, __FILE__, __LINE__ + 1)
      ->(unpacker) {
        #{struct}.new(#{args})
      }
    RUBY
  end
end

module Coders
  def self.register_time(factory)
    factory.register_type(
      MessagePack::Timestamp::TYPE,
      Time,
      packer: MessagePack::Time::Packer,
      unpacker: MessagePack::Time::Unpacker,
    )
  end

  module Marshal
    def self.factory
      ::Marshal
    end

    def self.description
      "Marshal is what we currently use and is the baseline for comparison"
    end
  end

  module PlainRubyNaive
    def self.build_factory
      factory = MessagePack::Factory.new
      Coders.register_time(factory)

      type_id = 0x20
      ALL_STRUCTS.each do |struct|
        factory.register_type(
          type_id,
          struct,
          packer: CodeGen.build_untracked_packer(struct),
          unpacker: CodeGen.build_untracked_unpacker(struct),
          recursive: true,
        )
        type_id += 1
      end

      factory
    end

    def self.factory
      @factory ||= build_factory
    end

    def self.description
      "A naive pure-Ruby coder without any optimizations to the msgpack-ruby gem, and no ref_tracking."
    end
  end

  module PlainRubyArrays
    def self.build_factory
      factory = MessagePack::Factory.new
      Coders.register_time(factory)

      type_id = 0x20
      ALL_STRUCTS.each do |struct|
        unpacker = eval(<<~RUBY, binding, __FILE__, __LINE__ + 1)
          -> (unpacker) {
            #{struct.members.join(", ")} = unpacker.read 
            #{struct}.new(#{struct.members.map{ |m| "#{m}: #{m}" }.join(", ")})
          }
        RUBY

        factory.register_type(
          type_id,
          struct,
          packer: -> (obj, packer) {
            packer.write(obj.to_a)
          },
          unpacker:,
          recursive: true,
        )
        type_id += 1
      end

      factory
    end

    def self.factory
      @factory ||= build_factory
    end

    def self.description
      "A pure-Ruby coder that uses array-based unpacking without ref_tracking."
    end
  end

  module PlainRubyRefTracking
    def self.build_factory
      factory = MessagePack::Factory.new
      Coders.register_time(factory)

      type_id = 0x20
      ALL_STRUCTS.each do |struct|
        factory.register_type(
          type_id,
          struct,
          packer: CodeGen.build_tracked_packer(struct),
          unpacker: CodeGen.build_tracked_unpacker(struct),
          recursive: true,
        )
        type_id += 1
      end

      factory
    end

    def self.factory
      @factory ||= build_factory
    end

    def self.description
      "A pure-Ruby coder without any optimizations to the msgpack-ruby gem, but with reference tracking implemented in Ruby."
    end
  end

  module OptimizedStruct
    def self.build_factory
      factory = MessagePack::Factory.new
      Coders.register_time(factory)

      type_id = 0x20
      ALL_STRUCTS.each do |struct|
        factory.register_type(type_id, struct, optimized_struct: true)
        type_id += 1
      end

      factory
    end

    def self.factory
      @factory ||= build_factory
    end

    def self.description
      "Uses msgpack-ruby's optimized_struct without ref_tracking. The lack of ref_tracking can lead to size bloat."
    end
  end

  module HybridRuby
    module PackerExt
      def ref_tracker
        @ref_tracker ||= {}
      end
    end

    module UnpackerExt
      def ref_tracker
        @ref_tracker ||= []
      end
    end

    MessagePack::Packer.include(PackerExt)
    MessagePack::Unpacker.include(UnpackerExt)

    def self.build_factory
      factory = MessagePack::Factory.new
      Coders.register_time(factory)

      type_id = 0x20
      ALL_STRUCTS.each do |struct|
        if SHARED_STRUCTS.include?(struct)
          # Ruby callbacks with ref_tracking for shared types
          factory.register_type(
            type_id,
            struct,
            packer: CodeGen.build_tracked_packer(struct),
            unpacker: CodeGen.build_tracked_unpacker(struct),
            recursive: true,
          )
        else
          # C-level optimized_struct for non-shared types
          factory.register_type(type_id, struct, optimized_struct: true)
        end
        type_id += 1
      end

      factory
    end

    def self.factory
      @factory ||= build_factory
    end

    def self.description
      "Uses optimized_struct for non-referenced Structs, and Ruby-level reference tracking for shared types"
    end
  end

  module RefTrackingInC
    def self.build_factory
      factory = MessagePack::Factory.new
      Coders.register_time(factory)

      type_id = 0x20
      ALL_STRUCTS.each do |struct|
        if SHARED_STRUCTS.include?(struct)
          factory.register_type(
            type_id,
            struct,
            optimized_struct: true,
            ref_tracking: true,
          )
        else
          factory.register_type(type_id, struct, optimized_struct: true)
        end
        type_id += 1
      end

      factory
    end

    def self.factory
      @factory ||= build_factory
    end

    def self.description
      "Uses optimized_struct with C-level reference tracking for shared types"
    end
  end
end

# =============================================================================
# Data Generation
# =============================================================================

def create_selling_plan(id:)
  SellingPlan.new(
    id: id,
    name: "Subscribe & Save #{id}",
    description: "Save 10% with a subscription",
    recurring_deliveries: true,
    options: [
      SellingPlanOption.new(name: "Delivery Frequency", position: 1, value: "1 Month"),
    ],
    price_adjustments: [
      SellingPlanPriceAdjustment.new(order_count: nil, position: 1, value_type: "percentage", value: 10),
    ],
    checkout_charge: SellingPlanCheckoutCharge.new(value_type: "percentage", value: 100),
  )
end

def create_selling_plan_group(id:, selling_plans:)
  SellingPlanGroup.new(
    id: id,
    name: "Subscription Group #{id}",
    options: [{ name: "Delivery Frequency", position: 1, values: ["1 Month", "2 Months"] }],
    selling_plans: selling_plans,
  )
end

def create_metafields(owner_id:, count:, owner_type:)
  # Match real-world metafield characteristics:
  # - Short, often repeated type values
  # - Some nil fields (definition_id, original_type)
  # - Relatively short values
  (1..count).map do |i|
    Metafield.new(
      id: owner_id * 1000 + i,
      namespace: "custom",
      key: "field_#{i}",
      value: "Value #{i}",
      type: "single_line_text_field", # this should be an enum
      value_type: "string",
      definition_id: nil,
      owner_type: owner_type,
      owner_id: owner_id,
      created_at: Time.now,
      updated_at: Time.now,
      original_type: nil,
    )
  end
end

def create_product(id:, num_variants:, num_options:, selling_plan_groups:, num_product_metafields:, num_variant_metafields:)
  # Create shared option values
  option_values_by_option = {}
  options = (1..num_options).map do |opt_idx|
    values = (1..3).map do |val_idx|
      ProductOptionValue.new(
        id: id * 1000 + opt_idx * 100 + val_idx,
        name: "Option#{opt_idx} Value#{val_idx}",
        position: val_idx,
        swatch_color: nil, # Most products don't have swatch colors
      )
    end
    option_values_by_option[opt_idx] = values

    ProductOption.new(
      id: id * 100 + opt_idx,
      name: "Option #{opt_idx}",
      position: opt_idx,
      values: values,
    )
  end

  # Create variants that SHARE the option values
  variants = (1..num_variants).map do |var_idx|
    # Each variant references the SAME ProductOptionValue objects
    variant_options = option_values_by_option.values.map { |vals| vals.sample }

    # Variants also SHARE the selling plans
    variant_selling_plans = selling_plan_groups.flat_map(&:selling_plans)

    # Match real ProductVariant structure with some nil fields (sparse data)
    ProductVariant.new(
      id: id * 1000 + var_idx,
      product_id: id,
      title: "Variant #{var_idx}",
      uncontextualized_title: nil,
      price: 1999 + var_idx * 100,
      compare_at_price: nil, # Most variants don't have compare_at_price
      barcode: nil, # Most variants don't have barcodes
      options: variant_options,
      option1: variant_options[0]&.name,
      option2: variant_options[1]&.name,
      option1_id: variant_options[0]&.id,
      option2_id: variant_options[1]&.id,
      taxable: true,
      position: var_idx,
      created_at: Time.now,
      updated_at: Time.now,
      fulfillment_service: "manual",
      requires_components: false,
      inventory_management: "shopify",
      inventory_policy: "deny",
      weight_unit: "kg",
      weight_value: nil,
      sku: "SKU-#{id}-#{var_idx}",
      requires_shipping: true,
      selling_plans: variant_selling_plans,
      metafields: create_metafields(owner_id: id * 1000 + var_idx, count: num_variant_metafields, owner_type: "ProductVariant"),
      variant_unit_price_measurement: nil,
    )
  end

  # Match real Product structure with some nil fields (sparse data)
  Product.new(
    id: id,
    title: "Product #{id}",
    handle: "product-#{id}",
    description: "Description for product #{id}",
    vendor: "Vendor",
    published_at: Time.now,
    created_at: Time.now,
    updated_at: Time.now,
    template_suffix: nil,
    gift_card: false,
    is_published: true,
    requires_selling_plan: selling_plan_groups.any?,
    published_scope: :published_scope_global,
    variants: variants,
    options: options,
    selling_plan_groups: selling_plan_groups,
    metafields: create_metafields(owner_id: id, count: num_product_metafields, owner_type: "Product"),
  )
end

def create_test_data(num_products:, num_variants:, num_selling_plan_groups:, num_selling_plans_per_group:, num_product_metafields: 0, num_variant_metafields: 0)
  # Create SHARED selling plans - same instances used across all products
  selling_plan_id = 1
  selling_plan_groups = (1..num_selling_plan_groups).map do |group_idx|
    selling_plans = (1..num_selling_plans_per_group).map do
      plan = create_selling_plan(id: selling_plan_id)
      selling_plan_id += 1
      plan
    end
    create_selling_plan_group(id: group_idx, selling_plans: selling_plans)
  end

  # Create products that share the same selling plan groups
  products = (1..num_products).map do |product_id|
    create_product(
      id: product_id,
      num_variants: num_variants,
      num_options: 2,
      selling_plan_groups: selling_plan_groups,
      num_product_metafields: num_product_metafields,
      num_variant_metafields: num_variant_metafields,
    )
  end

  products
end

# =============================================================================
# Benchmark
# =============================================================================

SCENARIOS = [
  # Baseline: minimal
  { products: 1, variants: 1, product_metafields: 0, variant_metafields: 0, spg: 0, sp: 0 },

  # Scale products
  { products: 50, variants: 1, product_metafields: 0, variant_metafields: 0, spg: 0, sp: 0 },

  # Scale variants
  { products: 10, variants: 50, product_metafields: 0, variant_metafields: 0, spg: 0, sp: 0 },

  # Scale product metafields
  { products: 10, variants: 5, product_metafields: 20, variant_metafields: 0, spg: 0, sp: 0 },

  # Scale variant metafields
  { products: 10, variants: 5, product_metafields: 0, variant_metafields: 10, spg: 0, sp: 0 },

  # Scale selling plans for ref_tracking
  { products: 10, variants: 5, product_metafields: 0, variant_metafields: 0, spg: 1, sp: 2 },
  { products: 10, variants: 5, product_metafields: 0, variant_metafields: 0, spg: 3, sp: 5 },

  # Combined
  { products: 10, variants: 10, product_metafields: 5, variant_metafields: 3, spg: 1, sp: 2 },
  { products: 50, variants: 250, product_metafields: 20, variant_metafields: 10, spg: 3, sp: 5 },
].freeze

Report = Struct.new(:scenario, :bench_results, :bytesize_results, keyword_init: true)

def run_benchmark(coders, scenario)
  data = create_test_data(
    num_products: scenario[:products],
    num_variants: scenario[:variants],
    num_selling_plan_groups: scenario[:spg],
    num_selling_plans_per_group: scenario[:sp],
    num_product_metafields: scenario[:product_metafields] || 0,
    num_variant_metafields: scenario[:variant_metafields] || 0,
  )

  puts "\nBenchmarking scenario: P:#{scenario[:products]} V:#{scenario[:variants]} PM:#{scenario[:product_metafields]} VM:#{scenario[:variant_metafields]} SPG:#{scenario[:spg]} SP:#{scenario[:sp]}"

  payloads = coders.map { |coder| coder.factory.dump(data) }

  result = Benchmark.ips(quiet: true) do |x|
    coders.each.with_index do |coder, index|
      x.report(coder.name.split("::").last) do
        coder.factory.load(payloads[index])
      end
    end
  end

  marshal_ips = result.entries.first.stats.central_tendency
  bench_results = result.entries.map.with_index do |entry, index|
    entry_ips = entry.stats.central_tendency
    speedup = entry_ips / marshal_ips
    [coders[index], speedup]
  end.to_h

  bytesize_results = coders.each_with_index.map do |coder, index|
    [coder, payloads[index].bytesize]
  end.to_h

  Report.new(
    scenario:,
    bench_results:,
    bytesize_results:,
  )
end

BOLD = "\e[1m"
RESET = "\e[0m"

def print_results(coders, reports)
  puts "=" * 80
  puts "BENCHMARK: Decode Performance & Size Comparison"
  puts "=" * 80
  puts "Coders:"
  coders.each do |coder|
    puts "  - #{coder.name.split("::").last}: #{coder.description}"
  end
  puts

  reports.each do |report|
    s = report.scenario
    scenario_str = "P:#{s[:products]} V:#{s[:variants]} PM:#{s[:product_metafields]} VM:#{s[:variant_metafields]} SPG:#{s[:spg]} SP:#{s[:sp]}"

    sorted_results = report.bench_results.sort_by { |_, v| -v }

    marshal_size = report.bytesize_results[Coders::Marshal]

    puts "Scenario: #{scenario_str}"
    puts "Winner: #{sorted_results.first[0].name.split("::").last} with #{'%.2f' % sorted_results.first[1]}x speedup"
    linesize = 56
    puts "-" * linesize
    puts format("%-20s %12s %10s %10s", "Coder", "Size (bytes)", "Size (%)", "Speedup")
    puts "-" * linesize

    sorted_results.each do |result|
      coder, speedup = result
      size = report.bytesize_results[coder]
      size_pct = (size.to_f / marshal_size) * 100
      coder_name = coder.name.split("::").last

      line = format("%-20s %12d %9.1f%% %9.2fx", coder_name, size, size_pct, speedup)
      if coder == Coders::Marshal
        puts "#{BOLD}#{line}#{RESET}"
      else
        puts line
      end
    end

    puts "=" * linesize
    puts
  end
end

# =============================================================================
# Main
# =============================================================================

puts "Running benchmarks..."
puts

coders = Coders.constants.map { |c| Coders.const_get(c) }
# Always put Marshal first for baseline comparison
coders.delete(Coders::Marshal)
coders.unshift(Coders::Marshal)

results = SCENARIOS.map.with_index do |scenario, i|
  print "\r[#{i + 1}/#{SCENARIOS.size}] Testing: P:#{scenario[:products]} V:#{scenario[:variants]} PM:#{scenario[:product_metafields]} VM:#{scenario[:variant_metafields]} SPG:#{scenario[:spg]} SP:#{scenario[:sp]}..."
  run_benchmark(coders, scenario)
end
puts "\r" + " " * 80 + "\r"

print_results(coders, results)
