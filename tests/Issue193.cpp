struct cppcmb_rule_tag16{};

template <typename Val, typename Tag>
class rule_t {};

#define cppcmb_prelude_cat(x, y) x ## y

#define cppcmb_cat(x, y) cppcmb_prelude_cat(x, y)

#define cppcmb_unique_id(prefix) cppcmb_cat(prefix, __LINE__)

#define cppcmb_decl(name, ...) \
auto const name =              \
rule_t<__VA_ARGS__, struct cppcmb_unique_id(cppcmb_rule_tag)>()

cppcmb_decl(expr_top, int);
