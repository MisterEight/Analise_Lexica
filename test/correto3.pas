{ Teste de operadores relacionais e estruturas aninhadas }
program Relacional;

var
   i, j, k : integer;

begin
   i := 1;
   j := 2;
   k := 3;

   if i <> j then
      if i <= k then
         k := k - i
      else
         k := k + j;

   while i < j do
   begin
      if j >= k then
         j := j - 1;
      i := i + 1
   end
end.
