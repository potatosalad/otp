-module(pt).

-export([start/0, loop/0]).

-define(X, 100).

start() ->
    Schedulers = erlang:system_info(schedulers),
    Put2 = spawn_multi(Schedulers, put_2, fun put_2/1, []),
    % ok = wait_multi(Put2),
    Erase1 = spawn_multi(Schedulers, erase_1, fun erase_1/1, []),
    Get1 = spawn_multi(Schedulers, get_1, fun get_1/1, []),
    Get2 = spawn_multi(Schedulers, get_2, fun get_2/1, []),
    % RevErase1 = spawn_multi(Schedulers, rev_erase_1, fun rev_erase_1/1, []),
    PutSame2 = spawn_multi(Schedulers, put_same_2, fun put_same_2/2, [?X div 2]),
    GetSame1 = spawn_multi(Schedulers, get_same_1, fun get_same_1/2, [?X div 2]),
    % ok = wait_multi(Erase1 ++ Get1 ++ Get2 ++ RevErase1),
    ok = wait_multi(Put2 ++ Erase1 ++ Get1 ++ Get2 ++ PutSame2 ++ GetSame1),
    ok.

loop() ->
    ok = start(),
    timer:sleep(10),
    loop().

spawn_multi(0, _Label, _Fun, _Args) ->
    [];
spawn_multi(N, Label, Fun, Args) ->
    {Pid, Mon} = spawn_monitor(erlang, apply, [Fun, [N | Args]]),
    [{{Pid, Mon}, {Label, N}} | spawn_multi(N - 1, Label, Fun, Args)].

wait_multi([]) ->
    ok;
wait_multi(Children0) ->
    receive
        {'DOWN', Mon, process, Pid, Reason} ->
            case lists:keytake({Pid, Mon}, 1, Children0) of
                {value, {{Pid, Mon}, {Label, N}}, Children1} ->
                    case Reason of
                        normal -> ok;
                        _ -> io:format("~p ~p exited abnormally: ~p~n", [Label, N, Reason])
                    end,
                    wait_multi(Children1);
                false ->
                    wait_multi(Children0)
            end
    after 5000 ->
        io:format("No exits after 5 seconds...~n", []),
        wait_multi(Children0)
    end.

put_2(I) ->
    put_2(I, ?X, rand:lcg35(erlang:unique_integer())).

put_2(_, 0, _S) ->
    exit(normal);
put_2(I, X, S) ->
    persistent_term:put({I, X}, {0, X, S}),
    persistent_term:put(X, {I, X, S}),
    put_2(I, X - 1, rand:lcg35(S)).

put_same_2(I, V) ->
    put_same_2(I, V, ?X, rand:lcg35(erlang:unique_integer())).

put_same_2(_, _V, 0, _S) ->
    exit(normal);
put_same_2(I, V, X, S) ->
    persistent_term:put({I, V}, {s, 0, X, S}),
    persistent_term:put(V, {s, I, X, S}),
    put_same_2(I, V, X - 1, rand:lcg35(S)).

erase_1(I) ->
    erase_1(I, ?X).

erase_1(_, 0) ->
    exit(normal);
erase_1(I, X) ->
    _R1 = persistent_term:erase({I, X}),
    _R2 = persistent_term:erase(X),
    erase_1(I, X - 1).

get_1(I) ->
    get_1(I, ?X).

get_1(_, 0) ->
    exit(normal);
get_1(I, X) ->
    _R1 = catch persistent_term:get({I, X}),
    _R2 = catch persistent_term:get(X),
    get_1(I, X - 1).

get_same_1(I, V) ->
    get_same_1(I, V, ?X).

get_same_1(_, _V, 0) ->
    exit(normal);
get_same_1(I, V, X) ->
    _R1 = catch persistent_term:get({I, V}),
    _R2 = catch persistent_term:get(V),
    get_same_1(I, V, X - 1).

get_2(I) ->
    get_2(I, ?X).

get_2(_, 0) ->
    exit(normal);
get_2(I, X) ->
    _R1 = persistent_term:get({I, X}, X),
    _R2 = persistent_term:get(X, {I, X}),
    get_2(I, X - 1).

rev_erase_1(I) ->
    rev_erase_1(I, ?X).

rev_erase_1(_, 0) ->
    exit(normal);
rev_erase_1(I, X) ->
    _R1 = persistent_term:erase({I, ?X - X}),
    _R2 = persistent_term:erase(?X - X),
    rev_erase_1(I, X - 1).
