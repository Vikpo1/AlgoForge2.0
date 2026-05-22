USE algoforge;

INSERT INTO users (id, username, email)
VALUES (1, 'local_user', NULL)
ON DUPLICATE KEY UPDATE username = VALUES(username);

INSERT INTO problems (
  id,
  title,
  oj,
  url,
  difficulty,
  tags_json,
  statement_markdown,
  input_description,
  output_description,
  sample_input,
  sample_output
) VALUES
(101, '补题 A - 最短路', 'Codeforces', 'https://codeforces.com/problemset/problem/1/A', 'Medium', JSON_ARRAY('图论', '最短路'), '## 题面\n\n给定一张带权图，请求从起点到终点的最短路径。', '输入包含图的点数、边数以及所有边的信息。', '输出最短路径长度或最优答案。', '4 4\n1 2 3\n2 4 5\n1 3 2\n3 4 4', '6'),
(102, '补题 B - 贪心', 'AtCoder', 'https://atcoder.jp/contests/abc001/tasks/abc001_1', 'Easy', JSON_ARRAY('贪心', '排序'), '## 题面\n\n根据题意做出若干次局部最优选择，求最终结果。', '输入为若干整数和约束条件。', '输出满足要求的方案或数值。', '3\n1 2 3', '2'),
(201, 'DP A - 背包', 'Luogu', 'https://www.luogu.com.cn/problem/P1048', 'Medium', JSON_ARRAY('DP', '背包'), '## 题面\n\n这是一个经典背包问题，请注意状态定义与转移方向。', '输入包括物品数量和容量上限。', '输出最大价值或最优方案。', '4 10\n2 3\n3 4\n4 5\n5 8', '12'),
(202, 'DP B - 区间 DP', 'Luogu', 'https://www.luogu.com.cn/problem/P1880', 'Hard', JSON_ARRAY('DP', '区间DP'), '## 题面\n\n请在区间长度递增的过程中完成状态转移。', '输入为区间相关的序列数据。', '输出区间 DP 的最优值。', '5\n1 2 3 4 5', '9')
ON DUPLICATE KEY UPDATE
  title = VALUES(title),
  oj = VALUES(oj),
  difficulty = VALUES(difficulty),
  tags_json = VALUES(tags_json),
  statement_markdown = VALUES(statement_markdown),
  input_description = VALUES(input_description),
  output_description = VALUES(output_description),
  sample_input = VALUES(sample_input),
  sample_output = VALUES(sample_output);

INSERT INTO problem_lists (id, user_id, name, description, list_user_weight)
VALUES
(1, 1, '赛后补题', '比赛后未解决题目', 80),
(2, 1, 'DP 专题', '动态规划专项训练', 20)
ON DUPLICATE KEY UPDATE
  name = VALUES(name),
  description = VALUES(description),
  list_user_weight = VALUES(list_user_weight);

INSERT IGNORE INTO problem_list_items (list_id, problem_id)
VALUES
(1, 101),
(1, 102),
(2, 201),
(2, 202);

INSERT INTO review_states (
  user_id,
  problem_id,
  status,
  problem_user_weight,
  review_count,
  last_feedback
) VALUES
(1, 101, 'FIRST_FIX', 1, 0, 'FAILED'),
(1, 102, 'COOLING', 1, 1, 'SOLVED_WITH_HINT'),
(1, 201, 'TRAINING', 5, 2, 'SOLVED_SLOWLY'),
(1, 202, 'ARCHIVED', 100, 3, 'SOLVED_FAST')
ON DUPLICATE KEY UPDATE
  status = VALUES(status),
  problem_user_weight = VALUES(problem_user_weight),
  review_count = VALUES(review_count),
  last_feedback = VALUES(last_feedback);

INSERT INTO notes (user_id, problem_id, hint_markdown, note_markdown)
VALUES
(1, 101, '可以先思考图上边权和路径转移关系，不要急着看完整题解。', '### 解题复盘\n\n这道题可以抽象成最短路模型。\n\n关键点：\n\n1. 建图方式是否正确；\n2. 起点和终点是否明确；\n3. 是否存在多源或特殊边权；\n4. 根据边权选择 Dijkstra 或 BFS 变体。'),
(1, 102, '先观察局部最优选择是否会影响后续状态。', '### 解题复盘\n\n这道题偏贪心。\n\n核心是找到一个可以证明的局部最优策略，而不是只凭直觉选择。'),
(1, 201, '先定义 dp[i][j] 或 dp[i] 的含义，再考虑转移方向。', '### 解题复盘\n\n这是一道背包类 DP。\n\n建议先明确：\n\n1. 物品维度；\n2. 容量维度；\n3. 状态含义；\n4. 顺序枚举还是逆序枚举。'),
(1, 202, '区间 DP 一般先枚举区间长度，再枚举左端点。', '### 解题复盘\n\n这是一道区间 DP。\n\n常见套路：\n\n1. 枚举区间长度 len；\n2. 枚举左端点 l；\n3. 计算右端点 r；\n4. 枚举断点 k 进行状态转移。')
ON DUPLICATE KEY UPDATE
  hint_markdown = VALUES(hint_markdown),
  note_markdown = VALUES(note_markdown);
