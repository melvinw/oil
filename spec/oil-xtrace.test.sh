# Oil xtrace

#### xtrace_details doesn't show [[ ]] etc.
shopt -s oil:basic
set -x

dir=/
if [[ -d $dir ]]; then
  (( a = 42 ))
fi
cd /

## stdout-json: ""
## STDERR:
. builtin cd '/'
## END

#### xtrace_details AND xtrace_rich on
shopt -s oil:basic xtrace_details
shopt --unset errexit
set -x

{
  env false
  set +x
} 2>err.txt

sed --regexp-extended 's/[[:digit:]]{2,}/12345/g' err.txt >&2

## STDOUT:
## END
## STDERR:
| command 12345: env false
; process 12345: status 1
. builtin set '+x'
## END

#### proc and shell function
shopt --set oil:basic
set -x

shfunc() {
  : $1
}

proc p {
  : $1
}

shfunc 1
p 2
## stdout-json: ""
## STDERR:
[ proc shfunc 1
  . builtin ':' 1
] proc
[ proc p 2
  . builtin ':' 2
] proc
## END

#### eval
shopt --set oil:basic
set -x

eval 'echo 1; echo 2'
## STDOUT:
1
2
## END
## STDERR:
[ eval
  . builtin echo 1
  . builtin echo 2
] eval
## END

#### source
echo 'echo source-argv: "$@"' > lib.sh

shopt --set oil:basic
set -x

source lib.sh 1 2 3

## STDOUT:
source-argv: 1 2 3
## END
## STDERR:
[ source lib.sh 1 2 3
  . builtin echo 'source-argv:' 1 2 3
] source
## END

#### external and builtin
shopt --set oil:basic
shopt --unset errexit
set -x

{
  env false
  true
  set +x
} 2>err.txt

# normalize PIDs, assumed to be 2 or more digits
sed --regexp-extended 's/[[:digit:]]{2,}/12345/g' err.txt >&2

## stdout-json: ""
## STDERR:
| command 12345: env false
; process 12345: status 1
. builtin true
. builtin set '+x'
## END

#### subshell
shopt --set oil:basic
shopt --unset errexit
set -x

proc p {
  : p
}

{
  : begin
  ( 
    : 1
    p
    exit 3  # this is control flow, so it's not traced?
  )
  set +x
} 2>err.txt

sed --regexp-extended 's/[[:digit:]]{2,}/12345/g' err.txt >&2

## stdout-json: ""
## STDERR:
. builtin ':' begin
| forkwait 12345
  . 12345 builtin ':' 1
  [ 12345 proc p
    . 12345 builtin ':' p
  ] 12345 proc
; process 12345: status 3
. builtin set '+x'
## END

#### command sub
shopt --set oil:basic
set -x

{
  echo foo=$(echo bar)
  set +x

} 2>err.txt

sed --regexp-extended 's/[[:digit:]]{2,}/12345/g' err.txt >&2

## STDOUT:
foo=bar
## END
## STDERR:
| command sub 12345
  . 12345 builtin echo bar
; process 12345: status 0
. builtin echo 'foo=bar'
. builtin set '+x'
## END

#### process sub (nondeterministic)
shopt --set oil:basic
shopt --unset errexit
set -x

# we wait() for them all at the end

{
  : begin
  cat <(seq 2) <(echo 1)
  set +x
} 2>err.txt

sed --regexp-extended 's/[[:digit:]]{2,}/12345/g; s|/fd/.|/fd/N|g' err.txt >&2
#cat err.txt >&2

## STDOUT:
1
2
1
## END


# TODO: should we sort these lines to make it deterministic?

## STDERR:
. builtin ':' begin
| proc sub 12345
| proc sub 12345
  . 12345 exec seq 2
  . 12345 builtin echo 1
| command 12345: cat '/dev/fd/N' '/dev/fd/N'
; process 12345: status 0
; process 12345: status 0
; process 12345: status 0
. builtin set '+x'
## END

#### pipeline (nondeterministic)
shopt --set oil:basic
set -x

myfunc() {
  echo 1
  echo 2
}

{
  : begin
  myfunc | sort | wc -l
  set +x
} 2>err.txt

sed --regexp-extended 's/[[:digit:]]{2,}/12345/g; s|/fd/.|/fd/N|g' err.txt >&2

## STDOUT:
2
## END
## STDERR:
. builtin ':' begin
[ pipeline
  | part 12345
  | part 12345
    [ 12345 proc myfunc
    . 12345 exec sort
      . 12345 builtin echo 1
      . 12345 builtin echo 2
    ] 12345 proc
  | command 12345: wc -l
  ; process 12345: status 0
  ; process 12345: status 0
  ; process 12345: status 0
] pipeline
. builtin set '+x'
## END

#### singleton pipeline

# Hm extra tracing

shopt --set oil:basic
set -x

: begin
! false
: end

## stdout-json: ""
## STDERR:
. builtin ':' begin
[ pipeline
  . builtin false
] pipeline
. builtin ':' end
## END

#### Background pipeline (separate code path)

shopt --set oil:basic
shopt --unset errexit
set -x

myfunc() {
  echo 2
  echo 1
}

{
	: begin
	myfunc | sort | grep ZZZ &
	wait
	echo status=$?
  set +x
} 2>err.txt

sed --regexp-extended 's/[[:digit:]]{2,}/12345/g' err.txt >&2

## STDOUT:
status=0
## END
## STDERR:
. builtin ':' begin
| part 12345
| part 12345
  [ 12345 proc myfunc
| part 12345
. builtin wait
  . 12345 exec sort
    . 12345 builtin echo 2
    . 12345 builtin echo 1
  ] 12345 proc
  . 12345 exec grep ZZZ
; process 12345: status 0
; process 12345: status 0
; process 12345: status 1
. builtin echo 'status=0'
. builtin set '+x'
## END

#### Background process with fork and & (nondeterministic)
shopt --set oil:basic
set -x

{
  sleep 0.1 &
  wait

  shopt -s oil:basic

  fork {
    sleep 0.1
  }
  wait
  set +x
} 2>err.txt

sed --regexp-extended 's/[[:digit:]]{2,}/12345/g' err.txt >&2

## stdout-json: ""
## STDERR:
| fork 12345
. builtin wait
  . 12345 exec sleep 0.1
; process 12345: status 0
. builtin shopt -s 'oil:basic'
. builtin fork
| fork 12345
. builtin wait
  . 12345 exec sleep 0.1
; process 12345: status 0
. builtin set '+x'
## END

# others: redirects?

#### here doc
shopt --set oil:basic
shopt --unset errexit
set -x

{
  : begin
  tac <<EOF
3
2
EOF

  set +x
} 2>err.txt

sed --regexp-extended 's/[[:digit:]]{2,}/12345/g' err.txt >&2

## STDOUT:
2
3
## END
## STDERR:
. builtin ':' begin
| here doc 12345
| command 12345: tac
; process 12345: status 0
; process 12345: status 0
. builtin set '+x'
## END

#### Two here docs

# BUG: This trace shows an extra process?

shopt --set oil:basic
shopt --unset errexit
set -x

{
  cat - /dev/fd/3 <<EOF 3<<EOF2
xx
yy
EOF
zz
EOF2

  set +x
} 2>err.txt

sed --regexp-extended 's/[[:digit:]]{2,}/12345/g' err.txt >&2

## STDOUT:
xx
yy
zz
## END
## STDERR:
| here doc 12345
| here doc 12345
| command 12345: cat - '/dev/fd/3'
; process 12345: status 0
; process 12345: status 0
; process 12345: status 0
. builtin set '+x'
## END

#### Control Flow
shopt --set oil:basic
set -x

for i in 1 2 3 {
  echo $i
  if (i == '2') {
    break
  }
}
## STDOUT:
1
2
## END
## STDERR:
. builtin echo 1
. builtin echo 2
+ break
## END