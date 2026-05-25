CREATE DATABASE IF NOT EXISTS algoforge
  DEFAULT CHARACTER SET utf8mb4
  DEFAULT COLLATE utf8mb4_unicode_ci;

USE algoforge;

CREATE TABLE IF NOT EXISTS users (
  id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  username VARCHAR(64) NOT NULL,
  email VARCHAR(255) NULL,
  password_hash VARCHAR(255) NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (id),
  UNIQUE KEY uk_users_username (username),
  UNIQUE KEY uk_users_email (email)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS problems (
  id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  title VARCHAR(255) NOT NULL,
  oj VARCHAR(64) NOT NULL,
  url VARCHAR(768) NOT NULL,
  difficulty VARCHAR(64) NULL,
  source_problem_id VARCHAR(128) NULL,
  tags_json JSON NULL,
  statement_markdown MEDIUMTEXT NULL,
  input_description TEXT NULL,
  output_description TEXT NULL,
  sample_input TEXT NULL,
  sample_output TEXT NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (id),
  UNIQUE KEY uk_problems_url (url),
  KEY idx_problems_oj (oj)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS problem_lists (
  id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  user_id BIGINT UNSIGNED NOT NULL,
  name VARCHAR(128) NOT NULL,
  description TEXT NULL,
  list_user_weight INT NOT NULL DEFAULT 50,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (id),
  KEY idx_problem_lists_user_id (user_id),
  CONSTRAINT fk_problem_lists_user
    FOREIGN KEY (user_id) REFERENCES users(id)
    ON DELETE CASCADE,
  CONSTRAINT chk_problem_lists_weight
    CHECK (list_user_weight >= 0 AND list_user_weight <= 100)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS problem_list_items (
  list_id BIGINT UNSIGNED NOT NULL,
  problem_id BIGINT UNSIGNED NOT NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (list_id, problem_id),
  KEY idx_problem_list_items_problem_id (problem_id),
  CONSTRAINT fk_problem_list_items_list
    FOREIGN KEY (list_id) REFERENCES problem_lists(id)
    ON DELETE CASCADE,
  CONSTRAINT fk_problem_list_items_problem
    FOREIGN KEY (problem_id) REFERENCES problems(id)
    ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS review_states (
  user_id BIGINT UNSIGNED NOT NULL,
  problem_id BIGINT UNSIGNED NOT NULL,
  status VARCHAR(32) NOT NULL DEFAULT 'FIRST_FIX',
  problem_user_weight INT NOT NULL DEFAULT 1,
  review_count INT NOT NULL DEFAULT 0,
  last_feedback VARCHAR(32) NOT NULL DEFAULT 'FAILED',
  next_review_at DATETIME NULL,
  last_reviewed_at DATETIME NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (user_id, problem_id),
  KEY idx_review_states_next_review (user_id, next_review_at),
  KEY idx_review_states_status (user_id, status),
  CONSTRAINT fk_review_states_user
    FOREIGN KEY (user_id) REFERENCES users(id)
    ON DELETE CASCADE,
  CONSTRAINT fk_review_states_problem
    FOREIGN KEY (problem_id) REFERENCES problems(id)
    ON DELETE CASCADE,
  CONSTRAINT chk_review_states_problem_weight
    CHECK (problem_user_weight >= 0 AND problem_user_weight <= 100),
  CONSTRAINT chk_review_states_review_count
    CHECK (review_count >= 0),
  CONSTRAINT chk_review_states_status
    CHECK (status IN ('FIRST_FIX', 'COOLING', 'TRAINING', 'MASTERED', 'ARCHIVED')),
  CONSTRAINT chk_review_states_feedback
    CHECK (last_feedback IN ('FAILED', 'SOLVED_WITH_HINT', 'SOLVED_SLOWLY', 'SOLVED_FAST'))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS notes (
  id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  user_id BIGINT UNSIGNED NOT NULL,
  problem_id BIGINT UNSIGNED NOT NULL,
  hint_markdown TEXT NULL,
  note_markdown MEDIUMTEXT NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (id),
  UNIQUE KEY uk_notes_user_problem (user_id, problem_id),
  CONSTRAINT fk_notes_user
    FOREIGN KEY (user_id) REFERENCES users(id)
    ON DELETE CASCADE,
  CONSTRAINT fk_notes_problem
    FOREIGN KEY (problem_id) REFERENCES problems(id)
    ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS review_feedback_records (
  id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  user_id BIGINT UNSIGNED NOT NULL,
  problem_id BIGINT UNSIGNED NOT NULL,
  feedback VARCHAR(32) NOT NULL,
  duration_seconds INT NOT NULL DEFAULT 0,
  previous_status VARCHAR(32) NULL,
  next_status VARCHAR(32) NOT NULL,
  next_review_at DATETIME NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (id),
  KEY idx_feedback_records_user_problem (user_id, problem_id, created_at),
  CONSTRAINT fk_feedback_records_user
    FOREIGN KEY (user_id) REFERENCES users(id)
    ON DELETE CASCADE,
  CONSTRAINT fk_feedback_records_problem
    FOREIGN KEY (problem_id) REFERENCES problems(id)
    ON DELETE CASCADE,
  CONSTRAINT chk_feedback_records_feedback
    CHECK (feedback IN ('FAILED', 'SOLVED_WITH_HINT', 'SOLVED_SLOWLY', 'SOLVED_FAST')),
  CONSTRAINT chk_feedback_records_duration
    CHECK (duration_seconds >= 0)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS judge_submissions (
  id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  user_id BIGINT UNSIGNED NOT NULL,
  problem_id BIGINT UNSIGNED NOT NULL,
  language VARCHAR(64) NOT NULL,
  code MEDIUMTEXT NOT NULL,
  verdict VARCHAR(16) NOT NULL,
  runtime_ms INT NOT NULL DEFAULT 0,
  memory_kb INT NOT NULL DEFAULT 0,
  message VARCHAR(512) NULL,
  remote_judge VARCHAR(64) NULL,
  remote_submission_id VARCHAR(128) NULL,
  remote_submission_url VARCHAR(512) NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (id),
  KEY idx_judge_submissions_user_problem (user_id, problem_id, created_at),
  KEY idx_judge_submissions_remote (remote_judge, remote_submission_id),
  CONSTRAINT fk_judge_submissions_user
    FOREIGN KEY (user_id) REFERENCES users(id)
    ON DELETE CASCADE,
  CONSTRAINT fk_judge_submissions_problem
    FOREIGN KEY (problem_id) REFERENCES problems(id)
    ON DELETE CASCADE,
  CONSTRAINT chk_judge_submissions_verdict
    CHECK (verdict IN ('AC', 'WA', 'TLE', 'RE', 'CE'))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
