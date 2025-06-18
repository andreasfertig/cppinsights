#
# C++ Insights, copyright (C) by Andreas Fertig
# Distributed under an MIT license. See LICENSE for details
#
#------------------------------------------------------------------------------

if [ $(which insights) ]; then

_insights_clang_options()
{
  # Note: This implementation is derived from: https://github.com/llvm/llvm-project/blob/fe42d34274cac79794637bf2f69f85537dde8b74/clang/utils/bash-autocomplete.sh
  # It is reduced to what's needed for completing Clangs options for C++ Insights.
  local cur cword arg options

  COMPREPLY=()
  cword=$COMP_CWORD
  cur="${COMP_WORDS[$cword]}"

  for i in `seq 1 $cword`; do
    arg="$arg${COMP_WORDS[$i-1]}"

    # Handle = which gets a special treatment from bash
    [[ $i != $cword && "${COMP_WORDS[$(($i))]}" != '=' ]] && arg="$arg,"
  done

  options=$( insights -- --autocomplete="$arg" 2>/dev/null | awk '{print $1}' | tr '\n' ' ' )

  if [[ "$options" == "" ||  "$options" == " " ]]; then
    [[ "$cur" == '=' || "$cur" == -*= ]] && cur=""
    COMPREPLY=( $( compgen -f "$cur" | sort ) )
  else
    [[ "${options: -1}" == '=' ]] && compopt -o nospace 2>/dev/null
    [[ "$cur" == '=' ]] && cur=""
    COMPREPLY=( $( compgen -W "$options" -- "$cur" ) )
  fi
}

_insights()
{
  local cur words cword

  cur="${COMP_WORDS[COMP_CWORD]}"
  words=("${COMP_WORDS[@]}")
  cword=$COMP_CWORD

  for (( i=0; i<${#words[@]}; i++ )); do
    if [[ ${words[i]} == "--" ]]; then
      local clang_args=("${words[@]:i+1}")
      COMP_LINE="${clang_args[*]}"
      COMP_POINT=$(( ${#COMP_LINE} ))
      COMP_WORDS=("${clang_args[@]}")
      COMP_CWORD=$(( ${#clang_args[@]} ))

      _insights_clang_options "${COMP_WORDS[1]}" "$2" "$3"

      return 0
    fi
  done

  COMP_WORDS=$words
  COMP_CWORD=$cword

  VALUES=$(insights --autocomplete)
  COMPREPLY=( $( compgen -W '-h --help --help-list --version -p --extra-arg --extra-arg-before $VALUES' -- "$cur" ) )

  # Expanding files if nothing was provided
  if [[ -z "$COMPREPLY" || "$cur" == "" ]]; then
    COMPREPLY=( $( compgen -f "$cur" | sort ) )
  fi

  return 0
}
complete -o bashdefault -o default -F _insights insights

fi

# ex: ts=4 sw=4 et filetype=sh
