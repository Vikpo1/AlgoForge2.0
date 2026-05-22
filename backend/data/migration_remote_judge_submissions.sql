ALTER TABLE judge_submissions
  ADD COLUMN remote_judge VARCHAR(64) NULL,
  ADD COLUMN remote_submission_id VARCHAR(128) NULL,
  ADD COLUMN remote_submission_url VARCHAR(512) NULL,
  ADD KEY idx_judge_submissions_remote (remote_judge, remote_submission_id);
